
// This is the IRA2020 specific heartbeat signal to announce a device, more then the NATS Ping/Pong it defines the device specific topic roots
void nats_announce()
{
  String announce_message = String("{\"mac_string\": \"") + mac_string + String("\",");   // Add MAC
  announce_message += String("\"IP\":\"") + String(WiFi.localIP()) + String("\",");       // Add IP
  announce_message += String("\"HWTYPE\":\"") + String("IRA2020") + String("\",");        // Add HW TYPE
  announce_message += String("\"HWREV\":\"") + String("Rev.01") + String("\",");          // Add HW board Rev
  announce_message += String("\"EXTMODE\":\"") + String(ext_mode) + String("\",");
  announce_message += String("\"MODE\":\"") + String(nats_mode) + String("\",");
  
  String name;
  // send the name back
  for(uint i = 0; i < dev_name_length; i++)
  {
    name += String(EEPROM.read(DEV_NAME + i));
  }
  announce_message += String("\"NAME\":\"") + name + String("\"}");

  // TODO: Add everything in EEPROM,  ...
  String announce_topic = String(NATS_ROOT_TOPIC) + String(".announce");
  nats.publish(announce_topic.c_str(), announce_message.c_str());
}

void nats_publish_status ()
{
  String status_topic = String(NATS_ROOT_TOPIC) + String(".") + mac_string + String(".status");
  long rssi = WiFi.RSSI();
  String status_message = String("{\"rssi\": \"") + String(rssi) + String("\"}");

  nats.publish(status_topic.c_str(), status_message.c_str());
}

void nats_publish_ext_mode(uint mode)
{
  String ext_mode_topic = String(NATS_ROOT_TOPIC) + String(".") + mac_string + String(".ext_mode");
  nats.publish(ext_mode_topic.c_str(), String(mode, DEC).c_str());
}

void nats_publish_ir(uint64_t value, uint32_t address, uint32_t command)
{
  String ir_topic = String(NATS_ROOT_TOPIC) + String(".") + mac_string + String(".ir");

  uint32_t value_high, value_low;

  value_high = ((uint32_t *) &value)[0];
  value_low = ((uint32_t *) &value)[1];

  String ir_message = String("{\"value_high\": \"") + String(value_high) + String("\",");
  ir_message += String("{\"value_low\": \"") + String(value_low) + String("\",");  
  ir_message += String("\"address\":\"") + String(address) + String("\",");    
  ir_message += String("\"command\":\"") + String(command) + String("\"}");

  nats.publish(ir_topic.c_str(), ir_message.c_str());
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

/* Accepts 3 bytes
2 bytes: parameter index
1 byte: parameter value
*/
void nats_config_handler(NATS::msg msg) {
  Serial.println("[NATS] Config Handler");

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

  uint16_t param_index = (dec_data[0] << 8) + dec_data[1];
  Serial.print("[NATS] Parameter Index: ");
  Serial.println(param_index);

  Serial.print("[NATS] Parameter Value: ");
  Serial.println(dec_data[2]);
  
  EEPROM.write(param_index, dec_data[2]);
  EEPROM.commit();

  nats.publish(msg.reply, "+OK"); 
}

/* 
This receives a complete DMX Frame of 512+1 bytes (allow SC to be set), 513 bytes are Base64 encoded
Currently the start code isn't used
*/
void nats_dmx_frame_handler(NATS::msg msg) {
  Serial.println("[NATS] DMX Frame Handler");

  if( (nats_mode == MODE_DMX_OUT) || (nats_mode == MODE_DMX_TO_PIXELS_W_IR) || (nats_mode == MODE_DMX_TO_PIXELS_WO_IR))   // need to add ext mode to this
  {
    Serial.print("[NATS] Message Data: ");
    Serial.println(msg.data);

    uint8_t dec_data[msg.size];
    uint16_t dec_length = decode_base64((unsigned char *) msg.data, dec_data);  // hopefully length = 513, if not we just overwrite the first section

    Serial.print("[NATS] Decoded Message Length: ");
    Serial.println(dec_length);

    if(dec_length > 513)
    {
      Serial.print("[NATS] Got too many bytes!");
      nats.publish(msg.reply, "NOK");   // Not in the right mode
    }
    else
    {
      Serial.print("[NATS] Decoded Message Data: ");
      for(int i = 0; i<dec_length; i++)
      {
        Serial.print(i);
        Serial.print(": ");
        Serial.print(dec_data[i],HEX);
        Serial.print(" ");

        DMX::Write(i, dec_data[i]);     // Let's write it away
      }
      Serial.println(" ");
    }
  }
  else
  {
    Serial.println("[NATS] not in a DMX mode, leaving");
    nats.publish(msg.reply, "NOK");   // Not in the right mode
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
    nats.publish(msg.reply, "NOK, Not in DMX mode");
  }
}

/* This routine is the NATS RGB data parser
2 bytes offset
2 byte: length
datablock: <length> * <#data> * <pixels>
#data comes from EEPROM memory
bv: 0x00 0x02 0x00 0x02 0xRR 0xGG 0xBB 0xRR 0xGG 0xBB
for 2 pixels with RGB (3data per pixel) with offset 2
this way we can send RGBA or RGBW pixels in a later phase
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

    uint16_t pix_offset = (dec_data[0] << 8) + dec_data[1];
    Serial.print("[NATS] RGB Pixel Data Offset: ");
    Serial.println(pix_offset);
    
    uint16_t pix_len = (dec_data[2] << 8) + dec_data[3];
    Serial.print("[NATS] RGB Per Pixel Datapoints: ");
    Serial.println(pix_len);                            // @TODO: This needs to be checked & used!, currently A and W are ditched

    if(pix_len > MAX_PIXELS)
    {
      Serial.println("[NATS] Too many pixels");
      nats.publish(msg.reply, "NOK, Too many pixels");   // Not in the right mode
      return;
    }

    if(pix_len == 0)
    {
      Serial.println("[NATS] Length is 0");
      nats.publish(msg.reply, "NOK, Length is 0");   // Not ok
      return;
    }

    for(int led_index = 0; led_index < pix_len; led_index++) // from 0 increment with #data per pixel
    {
      uint pixel = led_index + pix_offset;
      leds[pixel].r = dec_data[4 + (led_index*3)];                    // actual data starts at 4th byte
      leds[pixel].g = dec_data[4 + (led_index*3) + 1];
      leds[pixel].b = dec_data[4 + (led_index*3) + 2];   

      Serial.print("[NATS] RGB Pixel: ");
      Serial.print(led_index);
      Serial.print(", R:");
      Serial.print(leds[led_index].r);
      Serial.print(" G:");
      Serial.print(leds[led_index].g);
      Serial.print(" B:");
      Serial.println(leds[led_index].b);
    }
    Serial.print("[NATS] RGB Pixel Data Copied Over!");
    nats.publish(msg.reply, "+OK");  
  }
  else
  {
    Serial.println("[NATS] not in a RGB mode, leaving");
    nats.publish(msg.reply, "NOK, not in a RGB mode");   // Not in the right mode
  }
}

/* This routine is the NATS RGB data parser
2 bytes length
1 byte: #data per pixel
datablock: <length> * <#data> * <pixels>
bv: 0x00 0x02 0x03 0xRR 0xGG 0xBB 0xRR 0xGG 0xBB
for 2 pixels with RGB (3data per pixel)
this way we can send RGBA or RGBW pixels in a later phase
*/
void nats_old_rgb_frame_handler(NATS::msg msg) {
  Serial.println("[NATS] Old RGB Frame Handler");

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
    Serial.print("[NATS] RGB Pixel Data Length: ");
    Serial.println(pix_len);
    
    if(pix_len > MAX_PIXELS)
    {
      Serial.println("[NATS] Too many pixels");
      nats.publish(msg.reply, "NOK, Too many pixels");   // Not in the right mode
      return;
    }

    Serial.print("[NATS] RGB Per Pixel Datapoints: ");
    Serial.println(dec_data[2]);                            // @TODO: This needs to be checked & used!, currently A and W are ditched

    for(int led_index = 0; led_index < pix_len; led_index++) // from 0 increment with #data per pixel
    {
      leds[led_index].r = dec_data[3 + led_index*dec_data[2]];                    // actual data starts at 3th byte
      leds[led_index].g = dec_data[3 + led_index*dec_data[2] + 1];
      leds[led_index].b = dec_data[3 + led_index*dec_data[2] + 2];   

      Serial.print("[NATS] RGB Pixel: ");
      Serial.print(led_index);
      Serial.print(" R: ");
      Serial.print(leds[led_index].r);
      Serial.print(" G: ");
      Serial.print(leds[led_index].g);
      Serial.print(" B: ");
      Serial.println(leds[led_index].b);
    }
    Serial.print("[NATS] RGB Pixel Data Copied Over!");
    nats.publish(msg.reply, "+OK");  
  }
  else
  {
    Serial.println("[NATS] not in a RGB mode, leaving");

    nats.publish(msg.reply, "NOK, not in RGB mode");   // Not in the right mode
  }
}

/* This is the FX callback routine
Base64 encoded message = 
First Byte = FX selection
Second Byte = FX Speed
Third Byte = FX Crossfade
4Th Byte = FGND R
5Th = FGND G
6TH = FGND B
7TH = BGND R
8TH = BGND G
9TH = BGND B
*/
void nats_fx_handler(NATS::msg msg) {
  Serial.println("[NATS] FX Handler");

  if( (nats_mode == MODE_FX_TO_PIXELS_W_IR ) || (nats_mode == MODE_FX_TO_PIXELS_WO_IR) )   // need to add ext mode to this
  {
    uint8_t dec_data[msg.size];
    uint8_t dec_length = decode_base64((unsigned char *) msg.data, dec_data);     // @TODO: check on length!

    fx_select = dec_data[0];
    fx_speed = dec_data[1];
    fx_xfade = dec_data[2];

    fx_fgnd_r = dec_data[3];
    fx_fgnd_g = dec_data[4];
    fx_fgnd_b = dec_data[5];

    fx_bgnd_r = dec_data[6];
    fx_bgnd_g = dec_data[7];
    fx_bgnd_b = dec_data[8];
    
    Serial.print("[NATS] Selected FX:");
    Serial.println(fx_select);

    Serial.print("[NATS] Set Speed:");
    Serial.println(fx_speed);

    Serial.print("[NATS] Set XFade:");
    Serial.println(fx_xfade);

    Serial.print("[NATS] FGND R:");
    Serial.print(fx_fgnd_r);
    Serial.print(" G: ");
    Serial.print(fx_fgnd_b);
    Serial.print(" B: ");
    Serial.println(fx_fgnd_b);

    Serial.print("[NATS] BGND R:");
    Serial.print(fx_bgnd_r);
    Serial.print(" G: ");
    Serial.print(fx_bgnd_b);
    Serial.print(" B: ");
    Serial.println(fx_bgnd_b);

    EEPROM.write(FX_SELECT, fx_select);
    EEPROM.write(FX_SPEED, fx_speed);
    EEPROM.write(FX_XFADE, fx_xfade);

    EEPROM.write(FX_FGND_R, fx_fgnd_r);
    EEPROM.write(FX_FGND_G, fx_fgnd_g);
    EEPROM.write(FX_FGND_B, fx_fgnd_b);

    EEPROM.write(FX_BGND_R, fx_bgnd_r);
    EEPROM.write(FX_BGND_G, fx_bgnd_g);
    EEPROM.write(FX_BGND_B, fx_bgnd_b);

    EEPROM.commit();

    nats.publish(msg.reply, "+OK");   // Not in the right mode
  }
  else
  {
    Serial.println("[NATS] not in a FX mode, leaving");
    nats.publish(msg.reply, "NOK, not in FX mode");   // Not in the right mode
  }
}

/* This is the name callback routine
max 32 byte name message, ascii encoded
*/
void nats_name_handler(NATS::msg msg) {
  
  Serial.println("[NATS] Name Handler");

  if(msg.size > 32)
  {
    Serial.println("[NATS] Name too long, ignoring");
    nats.publish(msg.reply, "NOK, Name too long");
  }
  else if(msg.size == 0)
  {
    Serial.println("[NATS] Name request");
    String name;
    // send the name back
    for(uint i = 0; i < dev_name_length; i++)
    {
      name += String(EEPROM.read(DEV_NAME + i));
    }
    String nats_name_topic = String(NATS_ROOT_TOPIC) + String(".") + mac_string + String(".name");
    nats.publish(nats_name_topic.c_str(), name.c_str());
  }
  else
  {
    EEPROM.write(DEV_NAME_LENGTH, msg.size);
    dev_name_length = msg.size;

    // set the name
    Serial.print("[NATS] Name: ");
    for(uint c = 0; c < msg.size; c++)
    {
      Serial.print(msg.data[c]);
      EEPROM.write(DEV_NAME+c, msg.data[c]);
    }
    Serial.println(" ");
    EEPROM.commit();
    nats.publish(msg.reply, "+OK");
  }
}