#include <Arduino.h>

#include <WiFi.h>
#include <ArduinoNats.h>

#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

#include <esp_dmx.h>

#include <ArduinoJson.h>
#include <ArduinoOTA.h>

#include "base64.hpp"
#include "ir_data_packet.h"

// GPIO PIN DEFINITION
#define DEBUG_LED 	15

#define IR_PIN      5

#define DMX_TX      13
#define DMX_RX      23
#define DMX_TEN     19
#define DMX_REN     18

#define PIXEL_DATA  2
#define PIXEL_CLK   4

#define MODE1		    34
#define MODE2		    35
#define MODE3		    36
#define MODE4		    39

//#define NATS_SERVER "demo.nats.io"
#define NATS_SERVER     "51.15.194.130" // NATS Server Daan
#define NATS_ROOT_TOPIC "area3001"
#define MAX_PIXELS      120

#include "eeprom_map.h"           // this keeps the map of the non-volatile storage & the variables belonging to it
#include "operation_modes.h"      // these define all possible operation modes of IRA2020
#include "spiramJSONDoc.h"
#include "FastLED.h"
#include "wifi_pass.h"

uint    ext_mode = 15;  // This is to read in the 4 mode configuration pins (external), defaults on 15 = no pins connected
String  mac_string;     // MAC Address in string format

WiFiClient client;
NATS nats(
	&client,
	NATS_SERVER, NATS_DEFAULT_PORT
);
IRrecv irrecv(IR_PIN);
decode_results results;
NATSUtil::NATSServer server;

CRGB leds[MAX_PIXELS];

#include "build_in_fx.h"
#include "nats_cb_handlers.h"

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

// Build the string that is used for NATS subscriptions
void wifi_setMACstring() {
  byte mac[6];
  WiFi.macAddress(mac);
  mac_string =  String(mac[5], HEX) + 
                String("_") +
                String(mac[4], HEX) + 
                String("_") +
                String(mac[3], HEX) + 
                String("_") +
                String(mac[2], HEX) + 
                String("_") +
                String(mac[1], HEX) + 
                String("_") +
                String(mac[0], HEX);
}

// Called if something goes wrong with the WIFI connection
void WiFiEvent(WiFiEvent_t event)
{
    Serial.printf("[WiFi-event] event: %d: ", event);

    switch (event) {
        case SYSTEM_EVENT_WIFI_READY: 
            Serial.println("WiFi interface ready");
            break;
        case SYSTEM_EVENT_SCAN_DONE:
            Serial.println("Completed scan for access points");
            break;
        case SYSTEM_EVENT_STA_START:
            Serial.println("WiFi client started");
            break;
        case SYSTEM_EVENT_STA_STOP:
            Serial.println("WiFi clients stopped");
            break;
        case SYSTEM_EVENT_STA_CONNECTED:
            Serial.println("Connected to access point");
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            Serial.println("Disconnected from WiFi access point");
            break;
        case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
            Serial.println("Authentication mode of access point has changed");
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            Serial.print("Obtained IP address: ");
            Serial.println(WiFi.localIP());
            break;
        case SYSTEM_EVENT_STA_LOST_IP:
            Serial.println("Lost IP address and IP address is reset to 0");
            break;
        case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
            Serial.println("WiFi Protected Setup (WPS): succeeded in enrollee mode");
            break;
        case SYSTEM_EVENT_STA_WPS_ER_FAILED:
            Serial.println("WiFi Protected Setup (WPS): failed in enrollee mode");
            break;
        case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
            Serial.println("WiFi Protected Setup (WPS): timeout in enrollee mode");
            break;
        case SYSTEM_EVENT_STA_WPS_ER_PIN:
            Serial.println("WiFi Protected Setup (WPS): pin code in enrollee mode");
            break;
        case SYSTEM_EVENT_AP_START:
            Serial.println("WiFi access point started");
            break;
        case SYSTEM_EVENT_AP_STOP:
            Serial.println("WiFi access point  stopped");
            break;
        case SYSTEM_EVENT_AP_STACONNECTED:
            Serial.println("Client connected");
            break;
        case SYSTEM_EVENT_AP_STADISCONNECTED:
            Serial.println("Client disconnected");
            break;
        case SYSTEM_EVENT_AP_STAIPASSIGNED:
            Serial.println("Assigned IP address to client");
            break;
        case SYSTEM_EVENT_AP_PROBEREQRECVED:
            Serial.println("Received probe request");
            break;
        case SYSTEM_EVENT_GOT_IP6:
            Serial.println("IPv6 is preferred");
            break;
        case SYSTEM_EVENT_ETH_START:
            Serial.println("Ethernet started");
            break;
        case SYSTEM_EVENT_ETH_STOP:
            Serial.println("Ethernet stopped");
            break;
        case SYSTEM_EVENT_ETH_CONNECTED:
            Serial.println("Ethernet connected");
            break;
        case SYSTEM_EVENT_ETH_DISCONNECTED:
            Serial.println("Ethernet disconnected");
            break;
        case SYSTEM_EVENT_ETH_GOT_IP:
            Serial.println("Obtained IP address");
            break;
        default: break;
    }}

// Set up the WIFI connection
void connect_wifi() {
  Serial.print("[WIFI] connecting to ");
  Serial.println(WIFI_SSID);

  Serial.print("with MAC: ");
  wifi_printMAC();
  Serial.println(" ");
  wifi_setMACstring();

  // Delete old config
  WiFi.disconnect(true);

  WiFi.onEvent(WiFiEvent);

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

bool parse_server(const char* msg) {
			//StaticJsonDocument<200> doc;
      DynamicJsonDocument doc(1000);
      //SpiRamJsonDocument doc(1000);

			DeserializationError error = deserializeJson(doc, msg);

			if (error) {
          Serial.print("[NATS] deserializeJson() failed: ");
          Serial.println(error.c_str());
          Serial.print("Capacity: ");
          Serial.println(doc.capacity());
  				return false;
			}
			else {
				server.server_id = doc["server_id"]; 	// "NBFUSPNOHFLIABATORIMNGJFPFUK4M43PHP2EYJ2O2LRXK6F27PS7EXI"
				server.server_name = doc["server_name"];
				server.server_version = doc["version"]; 	// "2.6.5"
				server.proto = doc["proto"];				// 1
				server.go_version = doc["go"];			 	// "go1.17.2"
				server.server_host = doc["host"]; 			// "0.0.0.0"
				server.port = doc["port"]; 					// 4222
				server.headers = doc["headers"]; 			// true
				server.max_payload = doc["max_payload"]; 	// 1048576
				server.client_id = doc["client_id"]; 		// 6
				server.client_ip = doc["client_ip"]; 		// "192.168.20.238"	
			}
			return true;
}

void nats_print_server_info() {
  Serial.print("[SYS] nats_print_server_info() running on core ");
  Serial.println(xPortGetCoreID());

  Serial.println("[NATS] Server INFO: ");
  Serial.print("[NATS] Server ID: ");
  Serial.println(server.server_id);

  Serial.print("[NATS] Server Name: ");
  Serial.println(server.server_name);

  Serial.print("[NATS] Server Version: ");
  Serial.println(server.server_version);

  Serial.print("[NATS] Proto: ");
  Serial.println(server.proto);

  Serial.print("[NATS] Go Version: ");
  Serial.println(server.go_version);

  Serial.print("[NATS] Server Host: ");
  Serial.println(server.server_host);

  Serial.print("[NATS] Server Port: ");
  Serial.println(server.port);

  Serial.print("[NATS] Headers: ");
  Serial.println(server.headers);

  Serial.print("[NATS] Max Payload: ");
  Serial.println(server.max_payload);

  Serial.print("[NATS] Client ID: ");
  Serial.println(server.client_id);

  Serial.print("[NATS] Client IP: ");
  Serial.println(server.client_ip);
}

void nats_on_connect(const char* msg) {
  Serial.println("[NATS] Connect");
  Serial.println(msg);

  if(parse_server(msg))
  {
    nats_print_server_info();
  }

  String nats_debug_blink_topic = String(NATS_ROOT_TOPIC) + String(".") + mac_string + String(".blink");
  Serial.print("[NATS] Subscribing: ");
  Serial.println(nats_debug_blink_topic);
  nats.subscribe(nats_debug_blink_topic.c_str(), nats_debug_blink_handler);

  String nats_mode_topic = String(NATS_ROOT_TOPIC) + String(".") + mac_string + String(".mode");
  Serial.print("[NATS] Subscribing: ");
  Serial.println(nats_mode_topic);
  nats.subscribe(nats_mode_topic.c_str(), nats_mode_handler);

  String nats_ping_topic = String(NATS_ROOT_TOPIC) + String(".ping");
  Serial.print("[NATS] Subscribing: ");
  Serial.println(nats_ping_topic);
  nats.subscribe(nats_ping_topic.c_str(), nats_ping_handler);
  
  String nats_reset_topic = String(NATS_ROOT_TOPIC) + String(".") + mac_string + String(".reset");
  Serial.print("[NATS] Subscribing: ");
  Serial.println(nats_reset_topic);
  nats.subscribe(nats_reset_topic.c_str(), nats_reset_handler);

  String nats_config_topic = String(NATS_ROOT_TOPIC) + String(".") + mac_string + String(".config");
  Serial.print("[NATS] Subscribing: ");
  Serial.println(nats_config_topic);
  nats.subscribe(nats_config_topic.c_str(), nats_config_handler);

  //@TODO the following only need to subscribe upon mode change!

  String nats_dmx_topic = String(NATS_ROOT_TOPIC) + String(".") + mac_string + String(".dmx");
  Serial.print("[NATS] Subscribing: ");
  Serial.println(nats_dmx_topic);
  nats.subscribe(nats_dmx_topic.c_str(), nats_dmx_frame_handler);

  String nats_delta_dmx_topic = String(NATS_ROOT_TOPIC) + String(".") + mac_string + String(".deltadmx");
  Serial.print("[NATS] Subscribing: ");
  Serial.println(nats_delta_dmx_topic);
  nats.subscribe(nats_delta_dmx_topic.c_str(), nats_dmx_delta_frame_handler);

  String nats_rgb_topic = String(NATS_ROOT_TOPIC) + String(".") + mac_string + String(".rgb");
  Serial.print("[NATS] Subscribing: ");
  Serial.println(nats_rgb_topic);
  nats.subscribe(nats_rgb_topic.c_str(), nats_rgb_frame_handler);

  String nats_fx_topic = String(NATS_ROOT_TOPIC) + String(".") + mac_string + String(".fx");
  Serial.print("[NATS] Subscribing: ");
  Serial.println(nats_fx_topic);
  nats.subscribe(nats_fx_topic.c_str(), nats_fx_handler);

  String nats_name_topic = String(NATS_ROOT_TOPIC) + String(".") + mac_string + String(".name");
  Serial.print("[NATS] Subscribing: ");
  Serial.println(nats_name_topic);
  nats.subscribe(nats_name_topic.c_str(), nats_name_handler);

  nats_announce();
}

void nats_on_error(const char* msg) {
  Serial.print("[NATS] Error: ");
  Serial.println(msg);
  Serial.print(" outstanding pings: ");
  Serial.print(nats.outstanding_pings, DEC);
  Serial.print(" reconnect attempts: ");
  Serial.println(nats.reconnect_attempts, DEC);
}

void nats_on_disconnect() {
  Serial.println("[NATS] Disconnect");
}

/*
void base64test() 
{
  uint8_t length = 2+1+(3*4); // 2 length, 1 #pixeldata, 4pixels x 3 bytes/pixel
  
  Serial.print("[B64] Length: ");
  Serial.println(length);

  uint8_t input[length];  

  input[0] = 0;
  input[1] = 4;
  input[2] = 3;

  input[3] = 255; // pixel 1
  input[4] = 0;
  input[5] = 0;

  input[6] = 0; // pixel 2
  input[7] = 255;
  input[8] = 0;

  input[9] = 0; // pixel 3
  input[10] = 0;
  input[11] = 255;

  input[12] = 255; // pixel 4
  input[13] = 255;
  input[14] = 255;

  uint8_t output_length = 4* (ceil(length/3)) + 1; // +1 for null termination char

  Serial.print("[B64] Out Length: ");
  Serial.println(output_length); 

  uint8_t output[output_length];

  uint8_t encoded_length = encode_base64(input, length, output);

  Serial.print("[B64] Encoded Length: ");
  Serial.println(encoded_length); 

  Serial.print("[B64] Encoded: ");
  Serial.println((char *) output); 

  uint8_t decoded_length = length;   
  uint8_t decoded[decoded_length];

  unsigned int binary_length = decode_base64(output, decoded);

  Serial.print("[B64] Decoded Length: ");
  Serial.println(binary_length); 

  Serial.print("[B64] Decoded: ");
  for(int i = 0; i<length; i++)
  {
    Serial.print(decoded[i],HEX);
    Serial.print(" ");
  }
  Serial.println(" ");
}
*/ 

void setup_eeprom() {
    EEPROM.write(EEPROM_MAJ_VERSION, 1);
    EEPROM.write(EEPROM_MINOR_VERION, 3);
    EEPROM.write(PIXEL_LENGTH, 10);
    EEPROM.write(HW_BOARD_VERSION, 1);
    EEPROM.write(HW_BOARD_SERIAL_NR, 2);
    EEPROM.commit();
}

/*
void OTAhandleSketchDownload() {

  const char* SERVER = "10.2.0.1"; // must be string for HttpClient
  const unsigned short SERVER_PORT = 80;
  const char* PATH = "/update-v%d.bin";
  const unsigned long CHECK_INTERVAL = 5000;

  static unsigned long previousMillis;

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis < CHECK_INTERVAL)
    return;
  previousMillis = currentMillis;

  WiFiClient transport;                                   // THIS IS AN ISSUE AS ESP32 WIFI doesn't implement the necessary calls
  // should normally be able to replace this with this example for HTTP: https://randomnerdtutorials.com/esp32-http-get-post-arduino/
  HttpClient client(transport, SERVER, SERVER_PORT);

  char buff[32];
  snprintf(buff, sizeof(buff), PATH, VERSION + 1);

  Serial.print("[OTA] Check for update file ");
  Serial.println(buff);

  client.get(buff);

  int statusCode = client.responseStatusCode();
  Serial.print("[OTA] Update status code: ");
  Serial.println(statusCode);
  if (statusCode != 200) {
    client.stop();
    return;
  }

  long length = client.contentLength();
  if (length == HttpClient::kNoContentLengthHeader) {
    client.stop();
    Serial.println("Server didn't provide Content-length header. Can't continue with update.");
    return;
  }
  Serial.print("Server returned update file of size ");
  Serial.print(length);
  Serial.println(" bytes");

  if (!InternalStorage.open(length)) {
    client.stop();
    Serial.println("There is not enough space to store the update. Can't continue with update.");
    return;
  }
  byte b;
  while (length > 0) {
    if (!client.readBytes(&b, 1)) // reading a byte with timeout
      break;
    InternalStorage.write(b);
    length--;
  }
  InternalStorage.close();
  client.stop();
  if (length > 0) {
    Serial.print("Timeout downloading update file at ");
    Serial.print(length);
    Serial.println(" bytes. Can't continue with update.");
    return;
  }

  Serial.println("Sketch update apply and reset.");
  Serial.flush();
  InternalStorage.apply(); // this doesn't return
}
*/

void led_to_white() {
  for(uint i = 0; i < pixel_length; i++)
  {
    leds[i].r = 255;                    
    leds[i].g = 255;
    leds[i].b = 255;
  }  
}

// This is purely based on the assumption that RGB pars etc show white when all channels are at full (often the case)
void dmx_to_full() {
  for(uint i = 1; i < 513; i++)
  {
    DMX::Write(i, 255);
  }
}

void setup() {

  /// SERIAL PORT
  Serial.begin(115200);
  Serial.println(" ");
  Serial.println("[SYS] booting ira firmware");

  // EEPROM
  EEPROM.begin(EEPROM_SIZE);
  eeprom_restate();             // read all values back from EEPROM upon startup

  Serial.print("[SYS] setup() running on core ");
  Serial.println(xPortGetCoreID());

  /// IO
  configure_IO();
  Serial.println("[IO] set debug led on");
  digitalWrite(DEBUG_LED, LOW);

  Serial.print("[IO] current mode: ");
  ext_mode = getMode();
  Serial.println(getMode(), DEC);

  /// IR
  irrecv.enableIRIn();  // Start the IR receiver
  Serial.print("[IRrecv] running and waiting for IR message on Pin ");
  Serial.println(IR_PIN);

  /// WIFI
  connect_wifi();

  // @TODO remove this hard limit from the code
  pixel_length = 10;

  /// FASTLED                                                 // @TODO This init should take place here, it should happen after a mode is chosen
  Serial.println("[PIX] Setting up pixeltape");
  FastLED.addLeds<NEOPIXEL, PIXEL_DATA>(leds, pixel_length);

  for(int i = 0; i < pixel_length; i++) {   
    leds[i] = CRGB::White;
    FastLED.show();
    //delay(500);
    FastLED.delay(50);
    leds[i] = CRGB::Black;
    FastLED.show();
  }

  /// DMX
  Serial.println("[DMX] Setting up DMX");
  DMX::Initialize(output, true);      // Output & Double Pins
  
  /*
  Serial.println("[DMX] Test CH1");
  DMX::Write(1, 255);
  delay(500);
  DMX::Write(2, 255);
  delay(500);
  DMX::Write(3, 255);
  delay(500);
  DMX::Write(4, 255);
  */

  Serial.print("[OTA] starting OTA service  ...");
  ArduinoOTA.begin(WiFi.localIP(), mac_string.c_str(), "FlyingPigs789*", InternalStorage);

  /// NATS
  Serial.print("[NATS] connecting to nats ...");
  nats.on_connect = nats_on_connect;
  nats.on_error = nats_on_error;
  nats.on_disconnect = nats_on_disconnect;

  if(nats.connect())
  {
    Serial.print(" connected to: ");
    Serial.println(NATS_SERVER);
  }
  else
  {
    Serial.println("NATS not connected!");
  }

  // base64test();
}

void loop() {
  /// CHECK FOR MODE CHANGE
  if (ext_mode != getMode())
  {
    Serial.print("[IO] Ext Mode changed: ");
    Serial.println(getMode(), DEC); 
    ext_mode = getMode();

    printMode(ext_mode);
    if(nats.connected)
    {
      nats_publish_ext_mode(ext_mode);
    }
    delay(300); //against sending too many state changes at once
  }

  /// CHECK IF WIFI IS STILL CONNECTED
	if (WiFi.status() != WL_CONNECTED)
  { 
    connect_wifi();
  }

	nats.process();
  yield();

  /// CHECK FOR IR DATA
  if (irrecv.decode(&results)) {
    Serial.print("[IR] in Basic:");
    Serial.println(resultToHumanReadableBasic(&results));
 
    Serial.print("[IR] in SC:");
    Serial.println(resultToSourceCode(&results));

    Serial.print("[IR] parsed:");
    serialPrintUint64(results.value, HEX);
    Serial.print(" ");

    Serial.print(results.address, HEX);
    Serial.print(" ");

    Serial.print(results.command, HEX);
    Serial.println(" ");

    // Need to add type, repeat, ...

    // @TODO: send this out on a NATS topic "ROOT_TOPIC + MAC + .ir"
    nats_publish_ir(results.value, results.address, results.command);

    irrecv.resume();  // Receive the next value
  }

  switch (nats_mode)
  {
  case MODE_RGB_TO_PIXELS_W_IR:
  case MODE_RGB_TO_PIXELS_WO_IR:
    FastLED.show();
    break;
  
  case MODE_FX_TO_PIXELS_W_IR:
  case MODE_FX_TO_PIXELS_WO_IR:
    process_build_in_fx();
    FastLED.show();
    break;

  case MODE_WHITE_PIXELS:
    led_to_white();
    dmx_to_full();        // only when previous mode was DMX OUT
    FastLED.show();
    break;

  case MODE_DMX_IN:   // @TODO, switch to in
    break;

  case MODE_DMX_OUT:  // @TODO, switch to out
    break;

  default:
    break;
  }

  // check for WiFi OTA updates
  ArduinoOTA.poll();

  // send all status info
  nats_publish_status();
}
