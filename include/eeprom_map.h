#include <EEPROM.h>

/// ALL EEPROM ADDRESS LOCATIONS ///
#define EEPROM_MAJ_VERSION  1   // List the major eeprom layout version
#define EEPROM_MINOR_VERION 2   // List the minor eeprom layout version
#define NATS_MODE           3   // the remote set operation mode (over NATS)
#define PIXEL_LENGTH        4   // the number of pixels in the pixeltape

#define FX_SELECT           5
#define FX_SPEED            6
#define FX_XFADE            7
#define FX_FGND_R           8
#define FX_FGND_G           9
#define FX_FGND_B           10
#define FX_BGND_R           11
#define FX_BGND_G           12
#define FX_BGND_B           13

/// EEPROM SIZE
#define EEPROM_SIZE         13       // the number of bytes we want to read/store

/// ALL 'LIVE' Veriables that work with EEPROM non-volatile storage
uint8_t nats_mode;             // this variable keeps the operation mode set from NATS
bool    nats_mode_changed;     // indicate to the main loop that the mode changed
uint8_t pixel_length;

uint8_t fx_select;
uint8_t fx_speed;
uint8_t fx_xfade;
uint8_t fx_fgnd_r;
uint8_t fx_fgnd_g;
uint8_t fx_fgnd_b;
uint8_t fx_bgnd_r;
uint8_t fx_bgnd_g;
uint8_t fx_bgnd_b;

void eeprom_restate ()
{
    nats_mode   = EEPROM.read(NATS_MODE);
    pixel_length = EEPROM.read(PIXEL_LENGTH);

    fx_select   = EEPROM.read(FX_SELECT);
    fx_speed    = EEPROM.read(FX_SPEED);
    fx_xfade    = EEPROM.read(FX_XFADE);
    fx_fgnd_r   = EEPROM.read(FX_FGND_R);
    fx_fgnd_g   = EEPROM.read(FX_FGND_G);
    fx_fgnd_b   = EEPROM.read(FX_FGND_B);
    fx_bgnd_r   = EEPROM.read(FX_BGND_R);
    fx_bgnd_g   = EEPROM.read(FX_BGND_G);
    fx_bgnd_b   = EEPROM.read(FX_BGND_B);
}