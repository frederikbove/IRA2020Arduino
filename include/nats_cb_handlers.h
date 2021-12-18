
// This is the IRA2020 specific heartbeat signal to announce a device, more then the NATS Ping/Pong it defines the device specific topic roots
void nats_announce()
{
  String announce_message = String("{\"mac_string\": \"") + mac_string + String("\","); // Add MAC
  announce_message += String("\"IP\":\"") + String(WiFi.localIP()) + String("\"}");

  String announce_topic = String(NATS_ROOT_TOPIC) + String(".announce");
  nats.publish(announce_topic.c_str(), announce_message.c_str());
}

void nats_publish_ext_mode(uint mode)
{
  String ext_mode_topic = String(NATS_ROOT_TOPIC) + String(".") + mac_string + String(".ext_mode");
  nats.publish(ext_mode_topic.c_str(), String(mode, DEC).c_str());
}

void nats_ping_handler(NATS::msg msg) {
    Serial.println("[NATS] ping message received");

    delay(random(0,1000));      // random delay up to a sec to avoid broadcast storm

    nats_announce();
}

// This blinks the on-board debug LED a defined number of times (in the message) for board identification
void nats_debug_blink_handler(NATS::msg msg) {
  Serial.println("[NATS] debug led blink message received");
  nats.publish(msg.reply, "received!");

	int count = atoi(msg.data);
	while (count-- > 0) {
		digitalWrite(15, LOW);
		delay(100);
		digitalWrite(15, HIGH);
		delay(100);
	}
}

// This sets the operation mode of the board
void nats_mode_handler(NATS::msg msg) { 
  if(nats_mode != atoi(msg.data))
  {
    // TODO check if mode is valid
    Serial.print("[NATS] mode changed to: ");
    Serial.println(msg.data);

    nats_mode = atoi(msg.data);
    EEPROM.write(NATS_MODE, nats_mode);
    EEPROM.commit();

    printMode(nats_mode);
  }
  nats.publish(msg.reply, "+OK");
}

void nats_reset_handler(NATS::msg msg) { 
  Serial.println("[NATS] Reset CB Handler");
  if(atoi(msg.data))
  {
    Serial.println("[NATS] Restarting in 2 seconds");
 
    delay(2000);
 
    ESP.restart();
  }
  else
  {
    Serial.println("[NATS] Reset Handler, value was not >0");
  }
}

// This receives a complete DMX Frame of 512+1 bytes (allow SC to be set), 513 bytes are Base64 encoded
void nats_dmx_frame_handler(NATS::msg msg) {
  Serial.println("[NATS] DMX Frame Handler");

  if( (nats_mode == MODE_DMX_OUT) || (nats_mode == MODE_DMX_TO_PIXELS_W_IR) || (nats_mode == MODE_DMX_TO_PIXELS_WO_IR))   // need to add ext mode to this
  {
    Serial.print("[NATS] Message Data: ");
    Serial.println(msg.data);
  }
  else
  {
    Serial.println("[NATS] not in a DMX mode, leaving");
  }
}

// This receives a delta DMX Frame (normally smaller) of channel-value pairs
void nats_dmx_delta_frame_handler(NATS::msg msg) {
  Serial.println("[NATS] DMX Delta Frame Handler");

  if( (nats_mode == MODE_DMX_OUT) || (nats_mode == MODE_DMX_TO_PIXELS_W_IR) || (nats_mode == MODE_DMX_TO_PIXELS_WO_IR))   // need to add ext mode to this
  {
    Serial.print("[NATS] Message Data: ");
    Serial.println(msg.data);
  }
  else
  {
    Serial.println("[NATS] not in a DMX mode, leaving");
  }
}

/* ik dacht bericht als base64 door te sturen met hetvolgende:
2 bytes length
1 byte: #data per pixel
datablock: <length> * <#data> * <pixels>
bv: 0x00 0x02 0x03 0xRR 0xGG 0xBB 0xRR 0xGG 0xBB
voor 2 pixels met RGB (3data per pixel)
zodat we bv ook later RGBW or RGBA kunnen doorsturen
*/
void nats_rgb_frame_handler(NATS::msg msg) {
  Serial.println("[NATS] RGB Frame Handler");

  if( (nats_mode == MODE_RGB_TO_PIXELS_W_IR ) || (nats_mode == MODE_RGB_TO_PIXELS_WO_IR) )   // need to add ext mode to this
  {
    Serial.print("[NATS] Message Data: ");
    Serial.println(msg.data);

    uint8_t dec_data[msg.size];
    uint8_t dec_length = decode_base64((unsigned char *) msg.data, dec_data);

    Serial.print("[NATS] Decoded Message Length: ");
    Serial.println(dec_length);

    Serial.print("[NATS] Decoded Message Data: ");
    for(int i = 0; i<dec_length; i++)
    {
      Serial.print(dec_data[i],HEX);
      Serial.print(" ");
    }
    Serial.println(" ");

    uint16_t pix_len = (dec_data[0] << 8) + dec_data[1];
    Serial.print("[NATS] RGB Data Length: ");
    Serial.println(pix_len);

    Serial.print("[NATS] RGB Pixel datapoints: ");
    Serial.println(dec_data[2]);

    nats.publish(msg.reply, "+OK");  
  }
  else
  {
    Serial.println("[NATS] not in a RGB mode, leaving");

    nats.publish(msg.reply, "NOK");   // Not in the right mode
  }
}