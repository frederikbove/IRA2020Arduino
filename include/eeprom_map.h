#include <EEPROM.h>

/// ALL EEPROM ADDRESS LOCATIONS ///
#define NATS_MODE       1   // the remote set operation mode (over NATS)
#define PIXEL_LENGTH    2   // the number of pixels in the pixeltape

/// EEPROM SIZE
#define EEPROM_SIZE 2       // the number of bytes we want to read/store

/// ALL 'LIVE' Veriables that work with EEPROM non-volatile storage
uint nats_mode;             // this variable keeps the operation mode set from NATS
bool nats_mode_changed;     // indicate to the main loop that the mode changed
uint pixel_length;

void eeprom_restate ()
{
    nats_mode = EEPROM.read(NATS_MODE);
    pixel_length = EEPROM.read(PIXEL_LENGTH);
}