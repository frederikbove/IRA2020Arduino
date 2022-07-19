// Information below comes from: https://github.com/area3001/Timeblaster/blob/main/Firmware/data.h
// and from: https://github.com/area3001/Timeblaster/blob/main/Firmware/data.cpp
/*
enum IRTeamColor : uint8_t
{
  eNoTeam = 0b000,
  eTeamRex = 0b001,
  eTeamGiggle = 0b010,
  eTeamBuzz = 0b100,

  eTeamYellow = eTeamRex | eTeamGiggle,
  eTeamMagenta = eTeamRex | eTeamBuzz,
  eTeamCyan = eTeamGiggle | eTeamBuzz,

  eTeamWhite = eTeamRex | eTeamGiggle | eTeamBuzz
};

enum IRCommandType : uint8_t
{
  eCommandShoot = 1,
  eCommandHeal = 2,
  eCommandSetChannel = 3,
  eCommandSetFireType = 4,
  eCommandSetGameMode = 5,
  eCommandGotHit = 6,
  eCommandPlayAnimation = 7,
  eCommandTeamSwitched = 8,
  eCommandChatter = 9,
  eCommandPullTrigger = 10,
  eCommandSetFlagsA = 11,
  eCommandSetFlagsB = 12,
  eCommandReservedA = 13,
  eCommandReservedB = 14,
  eCommandBlasterAck = 15,
};

union IRDataPacket
{
  uint16_t raw;
  struct
  {
    IRTeamColor team : 3;
    bool trigger_state : 1;
    IRCommandType command : 4;
    uint8_t parameter : 4;
    uint8_t crc : 4;
  };
};

enum IRDeviceType: uint8_t
{
  eInfrared = 0b01,
  eBadge = 0b10,
  eAllDevices = 0b11,
};
*/
/* Calculate the 4-bit CRC and xor it with the existing CRC.
 * For new packages it add the CRC
 * For existing packages it will set the CRC to 0 if the existing CRC was correct.
 */
/*
IRDataPacket IRcalculateCRC(IRDataPacket packet)
{
  bool crc[] = {0, 0, 0, 0};
  // makes computing the checksum a litle bit faster
  bool d0 = bitRead(packet.raw, 0);
  bool d1 = bitRead(packet.raw, 1);
  bool d2 = bitRead(packet.raw, 2);
  bool d3 = bitRead(packet.raw, 3);
  bool d4 = bitRead(packet.raw, 4);
  bool d5 = bitRead(packet.raw, 5);
  bool d6 = bitRead(packet.raw, 6);
  bool d7 = bitRead(packet.raw, 7);
  bool d8 = bitRead(packet.raw, 8);
  bool d9 = bitRead(packet.raw, 9);
  bool d10 = bitRead(packet.raw, 10);
  bool d11 = bitRead(packet.raw, 11);

  crc[0] = d11 ^ d10 ^ d9 ^ d8 ^ d6 ^ d4 ^ d3 ^ d0 ^ 0;
  crc[1] = d8 ^ d7 ^ d6 ^ d5 ^ d3 ^ d1 ^ d0 ^ 1;
  crc[2] = d9 ^ d8 ^ d7 ^ d6 ^ d4 ^ d2 ^ d1 ^ 1;
  crc[3] = d10 ^ d9 ^ d8 ^ d7 ^ d5 ^ d3 ^ d2 ^ 0;

  bitWrite(packet.crc, 0, crc[0] ^ bitRead(packet.crc, 0));
  bitWrite(packet.crc, 1, crc[1] ^ bitRead(packet.crc, 1));
  bitWrite(packet.crc, 2, crc[2] ^ bitRead(packet.crc, 2));
  bitWrite(packet.crc, 3, crc[3] ^ bitRead(packet.crc, 3));

  return packet;
}
*/
bool IRvalidate_crc(uint16_t packet)
{
  bool crc[] = {0, 0, 0, 0};
  // makes computing the checksum a litle bit faster
  bool d0 = bitRead(packet, 0);
  bool d1 = bitRead(packet, 1);
  bool d2 = bitRead(packet, 2);
  bool d3 = bitRead(packet, 3);
  bool d4 = bitRead(packet, 4);
  bool d5 = bitRead(packet, 5);
  bool d6 = bitRead(packet, 6);
  bool d7 = bitRead(packet, 7);
  bool d8 = bitRead(packet, 8);
  bool d9 = bitRead(packet, 9);
  bool d10 = bitRead(packet, 10);
  bool d11 = bitRead(packet, 11);

  crc[0] = d11 ^ d10 ^ d9 ^ d8 ^ d6 ^ d4 ^ d3 ^ d0 ^ 0;
  crc[1] = d8 ^ d7 ^ d6 ^ d5 ^ d3 ^ d1 ^ d0 ^ 1;
  crc[2] = d9 ^ d8 ^ d7 ^ d6 ^ d4 ^ d2 ^ d1 ^ 1;
  crc[3] = d10 ^ d9 ^ d8 ^ d7 ^ d5 ^ d3 ^ d2 ^ 0;

  bitWrite(packet, 12, crc[0] ^ bitRead(packet, 12));
  bitWrite(packet, 13, crc[1] ^ bitRead(packet, 13));
  bitWrite(packet, 14, crc[2] ^ bitRead(packet, 14));
  bitWrite(packet, 15, crc[3] ^ bitRead(packet, 15));
  return ((packet & 0xf000) == 0);
}