
/* Core distribution
* DMX     Core 0
* Setup   Core 1
* Nats    Core
* IR      Core 
*/

#include <Arduino.h>

#include <WiFi.h>
#include <ArduinoNats.h>

#define _IR_ENABLE_DEFAULT_ false     // This has been moved to compiler arguments
#define DECODE_JVC          true

#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

#include <esp_dmx.h>

#include <ArduinoJson.h>

#include <HTTPClient.h>
#include <HTTPUpdate.h>

#include "base64.hpp"
#include "ir_data_packet.h"

#include <esp_partition.h>
#include "esp_ota_ops.h"          
#include "esp_http_client.h"
#include "esp_https_ota.h"

#define VERSION     5     // This is for HTTP based OTA, end user release versions tracker
/* Version History
* VERSION 1 : original HTTP OTA implementation
* VERSION 2 : intermediate version Daan
* VERSION 3 : final version DAAN
* VERSION 4 : disabled IR for WDT errors on some boards (Koen)
* VERSION 5 : trying to get IR fixed
*/

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

#define IR_DELAY    10000  // cycles the IR shows over the LEDs

//#define NATS_SERVER     "demo.nats.io"
//#define NATS_SERVER     "51.15.194.130" // NATS Server Daan ScaleWay
//#define NATS_SERVER     "10.2.0.2"        // NATS on PINKY (KB Design)
#define NATS_SERVER     "fri3d.triggerwear.io"
#define NATS_ROOT_TOPIC "area3001"
#define MAX_PIXELS      255       // @TODO this needs to increase, but then memory map needs to be adjusted

#include "eeprom_map.h"           // this keeps the map of the non-volatile storage & the variables belonging to it
#include "operation_modes.h"      // these define all possible operation modes of IRA2020
#include "spiramJSONDoc.h"
#include "FastLED.h"
#include "wifi_pass.h"

uint    ext_mode = 15;  // This is to read in the 4 mode configuration pins (external), defaults on 15 = no pins connected
String  mac_string;     // MAC Address in string format
uint    post = 0;       // counter to post status messages
uint    ir_delay = 0;   // counter to time the IR delay effect

uint8_t backup_fx_select;
uint8_t backup_fx_fgnd_r;
uint8_t backup_fx_fgnd_g;
uint8_t backup_fx_fgnd_b;
uint8_t backup_fx_bgnd_r;
uint8_t backup_fx_bgnd_g;
uint8_t backup_fx_bgnd_b;

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
#include "OTAStorage.h"


// Used to set the pinMode of each used pin and attach Interrupts where required
void configure_IO() {
	pinMode(DEBUG_LED, OUTPUT);

	pinMode(MODE1, INPUT_PULLUP);
	pinMode(MODE2, INPUT_PULLUP);
	pinMode(MODE3, INPUT_PULLUP);
	pinMode(MODE4, INPUT_PULLUP);
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
  Serial.print(WIFI_SSID);

  Serial.print(" with MAC: ");
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

  //Serial.print(" connected IP: ");
  //Serial.println(WiFi.localIP());
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

/* This routine is just to check the base64 math
*/
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

/* This routine is just to configure boards that have never booted
*/
void setup_eeprom() {
    EEPROM.write(EEPROM_MAJ_VERSION, 1);
    EEPROM.write(EEPROM_MINOR_VERION, 3);
    EEPROM.write(PIXEL_LENGTH, 10);
    EEPROM.write(HW_BOARD_VERSION, 1);
    EEPROM.write(HW_BOARD_SERIAL_NR, 2);
    EEPROM.commit();
}

void espOTA( String location)
{
  // Do the actual OTA
  esp_http_client_config_t config = {
    .url = location.c_str(),
  };
  config.auth_type = HTTP_AUTH_TYPE_NONE;
    /*
      esp_http_client_config_t config = {
        .url = location.c_str(),
        .host = SERVER,                     // this and following shouldn't be needed 
        .port = SERVER_PORT,
        .auth_type = HTTP_AUTH_TYPE_NONE,
        .path = path.c_str()
    };*/
  /*
  esp_https_ota_config_t ota_config = {
        .http_config = &config,
    };
  esp_err_t ret = esp_https_ota(&ota_config);
  */
  esp_err_t ret = esp_https_ota(&config); // OLD ESP-IDF

  switch (ret)
  {
    case ESP_OK:
      Serial.println("[OTA] ESP OTA SUCCESS");
      esp_restart();
      break;       // shouldn't execute this line normally
    case ESP_FAIL:
      Serial.println("[OTA] ESP OTA FAIL");
      break;
    case ESP_ERR_INVALID_ARG:
      Serial.println("[OTA] ESP OTA INVALID ARG");
      break;
    case ESP_ERR_OTA_VALIDATE_FAILED:
      Serial.println("[OTA] ESP OTA VALIDATE FAILED, invalid image");
      break; 
    case ESP_ERR_NO_MEM:
      Serial.println("[OTA] ESP OTA NO MEM");
      break; 
    case ESP_ERR_FLASH_OP_TIMEOUT:
      Serial.println("[OTA] ESP OTA FLASH OP TIMEOUT");
      break;
    case ESP_ERR_FLASH_OP_FAIL:
      Serial.println("[OTA] ESP OTA FLASH OP FAIL");
      break;
    default:
      Serial.println("[OTA] ESP OTA FAIL");
      break;
  }
}

/* This handles HTTP OTA
* Assumes a HTTP server runs on the network hosting the binary files
*/
void OTAhandleSketchDownload() {
  const unsigned long CHECK_INTERVAL = 60000;   // every 10min = 600000

  static unsigned long previousMillis;
  unsigned long currentMillis = millis();

  // Return out of this ASAP
  if (currentMillis - previousMillis < CHECK_INTERVAL)
    return;
  previousMillis = currentMillis;

  Serial.println("[OTA] HTTP OTA Handler check");

  const char* SERVER = "http://fri3d.triggerwear.io"; // must be string for HttpClient
  const unsigned short SERVER_PORT = 80;

  HTTPClient http;
  HTTPUpdate updater;
  
  String location;
  location += SERVER;
  location += ":";
  location += String(SERVER_PORT);
  location += "/update-v";
  location += String(VERSION + 1);    // If the next version is available
  location += ".bin";

  Serial.print("[OTA] location: ");
  Serial.println(location);             // gives: [OTA] location: 10.2.0.1:80/update-v2.bin

  http.begin(location);                 // We can also use the elaborate versionof begin 
  http.setAuthorization("arduino", "test");
  int httpResponseCode = http.GET();
  
  Serial.print("[OTA] HTTP Response code: ");
  Serial.println(httpResponseCode);    
  
  if (httpResponseCode != 200)          // 200 = HTTP OK
  {
    Serial.println("[OTA] Expected Code 200, exiting");
    // Free resources
    http.end();
    return;
  }

  int httpsize = http.getSize();
  Serial.print("[OTA] HTTP Size: ");
  Serial.print(httpsize);
  Serial.println(" bytes");

  int headers = http.headers();
  for(int i = 0; i < headers; i++)
  {
    Serial.print("[OTA] HTTP Header Name/Key: ");
    Serial.print(http.headerName(i));
    Serial.print(" Header Value: ");
    Serial.println(http.header(i));
  }

  uint32_t space = ESP.getFreeSketchSpace();
  Serial.print("[OTA] Free Sketch Space: ");
  Serial.println(space);

  const esp_partition_t* _partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, NULL);
  if(!_partition){
    Serial.println("[OTA] No partition available, exiting");
    return;
  }
  if(httpsize > _partition->size) {
    Serial.printf("[OTA] spiffsSize to low (%d) needed: %d\n", _partition->size, httpsize);
    return;
  }
  Serial.println("[OTA] Ready to update");

  char vstr[8];
  itoa(VERSION, vstr, 10);

  http.begin(location);
  updater.update(http, vstr);
  


  // Need to have a look here: https://github.com/espressif/arduino-esp32/blob/master/libraries/HTTPUpdate/src/HTTPUpdate.cpp
  // https://github.com/espressif/arduino-esp32/blob/master/libraries/HTTPUpdate/src/HTTPUpdate.h

  // Free resources 
  // http.end();
}

void all_led_to_color(uint8_t r, uint8_t g, uint8_t b) {
  for(uint i = 0; i < pixel_length; i++)
  {
    leds[i].r = r;                    
    leds[i].g = g;
    leds[i].b = b;
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
  Serial.print("[SYS] booting ira firmware, Version ");
  Serial.println(VERSION);

  // EEPROM
  EEPROM.begin(EEPROM_SIZE);
  setup_eeprom();
  eeprom_restate();             // read all values back from EEPROM upon startup
  eeprom_variables_print();

  if(dev_name_length > 32)    // only for empty platforms
  {
    dev_name_length = 32;
    EEPROM.write(DEV_NAME_LENGTH, dev_name_length);
    EEPROM.commit();
  }

  Serial.print("[SYS] setup() running on core ");
  Serial.println(xPortGetCoreID());

  /// IO
  configure_IO();
  Serial.println("[IO] set debug led on");
  digitalWrite(DEBUG_LED, LOW);

  Serial.print("[IO] current ext mode: ");
  ext_mode = getMode();
  Serial.println(getMode(), DEC);

  /// IR
  /*
  irrecv.enableIRIn();  // Start the IR receiver
  Serial.print("[IRrecv] running and waiting for IR message on Pin ");
  Serial.println(IR_PIN);
  */

  /// WIFI
  connect_wifi();

  /// FASTLED                                                 // @TODO This init shouldn't take place here, it should happen after a mode is chosen
  Serial.print("[PIX] Setting up pixeltape");
  Serial.println(pixel_length, DEC);
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
}

void loop() {
  /// CHECK FOR MODE CHANGE
  if (ext_mode != getMode())
  {
    Serial.print("[IO] Ext Mode changed: ");
    Serial.println(getMode(), DEC); 
    ext_mode = getMode();

    printMode(ext_mode);
    if (WiFi.status() == WL_CONNECTED)
    {
      if(nats.connected)
      {
        nats_publish_ext_mode(ext_mode);
      }
      delay(300);
    } //against sending too many state changes at once
  }

  /// CHECK IF WIFI IS STILL CONNECTED
	if (WiFi.status() != WL_CONNECTED)
  { 
    connect_wifi();
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    // make sure new messages are handled
	  nats.process();
    // check for HTTP OTA updates
    OTAhandleSketchDownload();

    // send all status info
    post++;
    if (post == 100000)
    {  
      nats_publish_status();
      post = 0;
    }
  }
  
  yield();        // Needed for FastLED

  /// CHECK FOR IR DATA
  // if (nats_mode == MODE_DMX_TO_PIXELS_W_IR || nats_mode == MODE_RGB_TO_PIXELS_W_IR || nats_mode == MODE_FX_TO_PIXELS_W_IR || nats_mode == MODE_AUTO_FX_W_IR) {
  if (irrecv.decode(&results)) {
    /*
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
    */

    uint16_t packet = (results.command & 0xff);
    packet <<= 8;
    packet += results.address & 0xff;

    if (IRvalidate_crc(packet)) 
    {
      Serial.println("CRC OK");

      // Overwrite the FX
      if(nats_mode == MODE_FX_TO_PIXELS_W_IR)
      {
        ir_delay = 1;
        // backup
        backup_fx_select = fx_select;
        backup_fx_fgnd_r = fx_fgnd_r;
        backup_fx_fgnd_g = fx_fgnd_g;
        backup_fx_fgnd_b = fx_fgnd_b;
        backup_fx_bgnd_r = fx_bgnd_r;
        backup_fx_bgnd_g = fx_bgnd_g;
        backup_fx_bgnd_b = fx_bgnd_b;

        fx_select = FX_PIXEL_LOOP;

        switch (results.address & 0b111) {
          case 1:
            fx_fgnd_r = 255;
            fx_fgnd_g = 0;
            fx_fgnd_b = 0;
            fx_bgnd_r = 255;
            fx_bgnd_g = 60;
            fx_bgnd_b = 60;
            break;
        case 2:
            fx_fgnd_r = 0;
            fx_fgnd_g = 255;
            fx_fgnd_b = 0;
            fx_bgnd_r = 60;
            fx_bgnd_g = 255;
            fx_bgnd_b = 60;
            break;
        case 4:
            fx_fgnd_r = 0;
            fx_fgnd_g = 0;
            fx_fgnd_b = 255;
            fx_bgnd_r = 60;
            fx_bgnd_g = 60;
            fx_bgnd_b = 255;
            break;
        }
      }
      // Overwrite the pixel content (no backup)
      if(nats_mode == MODE_RGB_TO_PIXELS_W_IR)      // in this case we don't back up, we just block updates for a while (memory footprint)
      {
        switch (results.address & 0b111) {
          case 1:
            all_led_to_color(255, 0, 0);
            break;
        case 2:
            all_led_to_color(0, 255, 0);
            break;
        case 4:
            all_led_to_color(0, 0, 255);
            break;
        }
      }
      if (WiFi.status() == WL_CONNECTED)
      {
        // Send out on NATS
        switch (results.address & 0b111) {
        case 1:
          Serial.println("REX");
          nats_publish_ir(packet, 1);
          break;
        case 2:
          Serial.println("GIGGLE");
          nats_publish_ir(packet, 2);
          break;
        case 4:
          Serial.println("BUZZ");
          nats_publish_ir(packet, 4);
          break;
        }
      }
    }
    irrecv.resume();  // Receive the next value
  }

  /// Main Processing based on mode
  switch (nats_mode)
  {
    case MODE_RGB_TO_PIXELS_W_IR:
      if(ir_delay != 0)
      {
        ir_delay++;
        if(ir_delay == 10000)
          ir_delay = 0;
      }
    case MODE_RGB_TO_PIXELS_WO_IR:
      FastLED.show();
      break;
  
    case MODE_FX_TO_PIXELS_W_IR:
      if(ir_delay != 0)
      {
        ir_delay++;
        if(ir_delay == 10000)
          ir_delay = 0;
      }
    case MODE_FX_TO_PIXELS_WO_IR:
      process_build_in_fx();
      FastLED.show();
      break;

    case MODE_WHITE_PIXELS:
      all_led_to_color(255, 255, 255);
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
}
