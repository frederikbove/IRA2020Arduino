#include <Arduino.h>

#include <WiFi.h>
#include <ArduinoNats.h>

const char* WIFI_SSID = "area3001_iot";
const char* WIFI_PSK = "hackerspace";

WiFiClient client;
NATS nats(
	&client,
	"demo.nats.io", NATS_DEFAULT_PORT
);

void connect_wifi() {
  Serial.print("connecting to ");
  Serial.println(WIFI_SSID);
	WiFi.mode(WIFI_STA);
	WiFi.begin(WIFI_SSID, WIFI_PSK, 4);
	while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    // wait 1 second for re-trying
    delay(1000);
  }

  Serial.println(" connected");
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
  nats.subscribe("area3001.blink", nats_blink_handler);
  nats.subscribe("area3001.control", control_protocol_handler);
}

void setup() {
  Serial.begin(9200);
  Serial.println("booting ira firmware");

	pinMode(15, OUTPUT);
	digitalWrite(15, HIGH);

	connect_wifi();

  Serial.print("connecting to nats ...");
	nats.on_connect = nats_on_connect;
	nats.connect();
  Serial.println(" connected");
}

void loop() {
	if (WiFi.status() != WL_CONNECTED) connect_wifi();
	nats.process();
	yield();
}

