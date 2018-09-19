#define MAX_MESSAGE_LEN 50

#include <SteamLink.h>
#include <SteamLinkLoRa.h>

#define VER "8"

#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3
#define LORALED 13
#define VBATPIN A7  

#define SL_ID 0x122

// minimum transmisison gap (ms)
#define MINTXGAP 125

// test LED and button
#define LED 5
#define BUTTON 6

#define  MIN(a,b) (((a)<(b))?(a):(b))
#define  MAX(a,b) (((a)>(b))?(a):(b))

struct SL_NodeCfgStruct config = {
  1,                  // version
  SL_ID,              // slid
  "Bathurst",          // name
  "P2P Workshop Node",     // description
  43.664,                  // gps_lat
  -79.411,               // gps_lon
  180,                // altitude
  150,                 // max_silence in s
  false,              // battery powered
  1                   // radio_params
  };

SteamLinkLoRa sl(&config);
struct SteamLinkLoRaConfig slconfig = { false, NULL, 0 };

/* Packet building */
char  data[100];
int packet_num = 0;

// button state
int8_t bLast, bCurrent = 2;

void sl_on_receive(uint8_t *buf, uint8_t len) ;

// Don't put this on the stack:
uint8_t buf[MAX_MESSAGE_LEN];
int beforeTime = 0, afterTime = 0, nextSendTime = 0;
int waitInterval = 90000;

int getBatInfo() {
    return int(analogRead(VBATPIN) * 6.45); // = *2*3.3/1024*1000
}

// Setup
void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.print(F("!ID SL_testclient " VER " slid "));
  Serial.println(SL_ID, HEX);

  pinMode(LED, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);

  digitalWrite(LED, HIGH);

  pinMode(LORALED, OUTPUT);

  sl.set_pins(RFM95_CS, RFM95_RST, RFM95_INT);
  sl.init((void *)&slconfig, sizeof(slconfig));
  sl.register_receive_handler(sl_on_receive);

  Serial.println("Steamlink init done");
  bLast = 2;
}

void loop()
{
  uint8_t len = sizeof(buf);

  sl.update();
  bCurrent = digitalRead(BUTTON);
  if ((millis() > nextSendTime) || (bCurrent != bLast)) {
    packet_num += 1;
    if (bCurrent != bLast) {
      int8_t value = (bCurrent == LOW ? 1 : 0);
      snprintf(data, sizeof(data), "Button %i pkt: %d", value, packet_num);
      bLast = bCurrent;
    } else {
      snprintf(data, sizeof(data), "Hello World! pkt: %d vBat: %dmV", packet_num, getBatInfo());
    }
    beforeTime = millis();

    digitalWrite(LORALED, HIGH);

    Serial.print("Sending \"");
    Serial.print(data);
    Serial.println("\"");
    bool rc = sl.send((uint8_t* )data);
    digitalWrite(LORALED, LOW);

    if (rc) {
      afterTime = millis() - beforeTime;
      // It has been reliably delivered to the next node.
      Serial.print("tx time: ");
      Serial.println(afterTime);
    } else {
      Serial.println("send failed");
    }
    nextSendTime = millis() + waitInterval;
  }
}

void sl_on_receive(uint8_t *buf, uint8_t len) {
    Serial.print("sl_on_receive: len: ");
    Serial.print(len);
    Serial.print(" msg: ");
    Serial.println((char*)buf);
    int v = atoi((char *)buf);
	if (v == 0) {
      digitalWrite(LED, LOW);
	} else if (v == 1) {
      digitalWrite(LED, HIGH);
	} else { 
      waitInterval = MAX(MINTXGAP, v);
      Serial.print("waitInterval now ");
      Serial.println(waitInterval);
    }
}
