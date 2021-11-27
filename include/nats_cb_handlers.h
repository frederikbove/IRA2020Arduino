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
  String ext_mode_topic = String(NATS_ROOT_TOPIC) + String(".ext_mode");
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
  }
  nats.publish(msg.reply, "+OK");
}