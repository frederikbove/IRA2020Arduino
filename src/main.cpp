#include <Arduino.h>

#include <WiFi.h>
#include <ArduinoNats.h>

#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

#include <dmx.h>

//#include "FastLED.h"
#include "wifi_pass.h"

#define DEBUG_LED 	15

#define IR_PIN      5

#define DMX_TX      17
#define DMX_RX      16
#define DMX_TEN     19
#define DMX_REN     18

#define PIXEL_DATA  2
#define PIXEL_CLK   4

#define MODE1		    34
#define MODE2		    35
#define MODE3		    36
#define MODE4		    39

#define NATS_SERVER "demo.nats.io"

int ext_mode = 15; // This is to read in the 4 mode configuration pins (external), defaults on 15 = no pins connected

WiFiClient client;
NATS nats(
	&client,
	NATS_SERVER, NATS_DEFAULT_PORT
);
IRrecv irrecv(IR_PIN);
decode_results results;

// Reads in the 4 pins that configure the operation mode of the unit and returns them as a single Mode value
int getMode() {
	/*
  Serial.print("Mode pin settings: ");
	Serial.print(digitalRead(MODE3), BIN);
	Serial.print(digitalRead(MODE4), BIN);
	Serial.print(digitalRead(MODE1), BIN);
	Serial.println(digitalRead(MODE2), BIN);
  */
	return (digitalRead(MODE2) + 2*digitalRead(MODE1) + 4*digitalRead(MODE4) + 8*digitalRead(MODE3));
}

/*
void IRAM_ATTR Mode_ISR() {
  if (getMode() != mode)
  {
    Serial.print("ISR! Mode changed: ");
    Serial.println(getMode(), DEC); 
    mode = getMode();
  };
}
*/

// Used to set the pinMode of each used pin and attach Interrupts where required
void configure_IO() {
	pinMode(DEBUG_LED, OUTPUT);

	pinMode(MODE1, INPUT_PULLUP);
	pinMode(MODE2, INPUT_PULLUP);
	pinMode(MODE3, INPUT_PULLUP);
	pinMode(MODE4, INPUT_PULLUP);

  /*
  attachInterrupt(MODE1, Mode_ISR, CHANGE);
  attachInterrupt(MODE2, Mode_ISR, CHANGE);
  attachInterrupt(MODE3, Mode_ISR, CHANGE);
  attachInterrupt(MODE4, Mode_ISR, CHANGE);
  */
}

// Gets the MAC address and prints it to serial Monitor
void wifi_printMAC() {
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print(mac[5], HEX);
  Serial.print(":");
  Serial.print(mac[4], HEX);
  Serial.print(":");
  Serial.print(mac[3], HEX);
  Serial.print(":");
  Serial.print(mac[2], HEX);
  Serial.print(":");
  Serial.print(mac[1], HEX);
  Serial.print(":");
  Serial.print(mac[0], HEX);
}

void connect_wifi() {
  Serial.print("connecting to ");
  Serial.println(WIFI_SSID);

  Serial.print("with MAC: ");
  wifi_printMAC();
  Serial.println(" ");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PSK, 4);
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    // wait 1 second for re-trying
    delay(1000);
  }

  Serial.print(" connected IP: ");
  Serial.println(WiFi.localIP());
}

void nats_blink_handler(NATS::msg msg) {
  Serial.println("nats message received");
  nats.publish(msg.reply, "received!");

	int count = atoi(msg.data);
	while (count-- > 0) {
		digitalWrite(15, LOW);
		delay(100);
		digitalWrite(15, HIGH);
		delay(100);
	}
}

void control_protocol_handler(NATS::msg msg) { 
  nats.publish(msg.reply, "+OK");
}

void nats_on_connect() {
  //Serial.println("Connect Blink");
  nats.subscribe("area3001.blink", nats_blink_handler);
  nats.subscribe("area3001.control", control_protocol_handler);
}

void nats_on_error() {
  Serial.print("ErrorL outstanding pings: ");
  Serial.print(nats.outstanding_pings, DEC);
  Serial.print(" reconnect attempts: ");
  Serial.println(nats.reconnect_attempts, DEC);
}

void setup() {

  /// SERIAL PORT
  Serial.begin(115200);
  Serial.println(" ");
  Serial.println("booting ira firmware");

  /// IO
  configure_IO();
  Serial.println("set debug led on");
  digitalWrite(DEBUG_LED, LOW);

  Serial.print("current mode: ");
  ext_mode = getMode();
  Serial.println(getMode(), DEC);

  /// IR
  irrecv.enableIRIn();  // Start the IR receiver
  Serial.print("IRrecvDemo is now running and waiting for IR message on Pin ");
  Serial.println(IR_PIN);

  /// WIFI
  connect_wifi();

  /// NATS
  Serial.print("connecting to nats ...");
  nats.on_connect = nats_on_connect;
  nats.on_error = nats_on_error;
  if(nats.connect())
  {
    Serial.print(" connected to: ");
    Serial.println(NATS_SERVER);
  }
  else
  {
    Serial.println("NATS not connected!");
  }
}

void loop() {
  /// CHECK FOR MODE CHANGE
  if (ext_mode != getMode())
  {
    Serial.print("Ext Mode changed: ");
    Serial.println(getMode(), DEC); 
    ext_mode = getMode();
  }

  /// CHECK IF WIFI IS STILL CONNECTED
	if (WiFi.status() != WL_CONNECTED)
  { 
    connect_wifi();
  }

	nats.process();
  //nats.publish("area3001.blink", "ping");
  yield();

  /// CHECK FOR IR DATA
   if (irrecv.decode(&results)) {
    serialPrintUint64(results.value, HEX);
    Serial.println("");
    irrecv.resume();  // Receive the next value
  }

}
