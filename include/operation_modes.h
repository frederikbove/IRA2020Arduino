#define MODE_EXT_SET                0   // When this mode is set the local 'external' pins set the operation mode
#define MODE_DMX_IN                 1   // RS485 DMX IN > NATS (DMX)
#define MODE_DMX_OUT                2   // NATS (DMX) > RS485 DMX OUT
#define MODE_DMX_TO_PIXELS_W_IR     3   // NATS (DMX) > PIXELTAPE    Nats sends pixel data as an DMX frame
#define MODE_DMX_TO_PIXELS_WO_IR    4   // NATS (DMX) > PIXELTAPE    Nats sends pixel data as an DMX frame
#define MODE_RGB_TO_PIXELS_W_IR     5   // NATS (RGB) > PIXELTAPE    Nats sends pixel data as an RGB framebuffer
#define MODE_RGB_TO_PIXELS_WO_IR    6   // NATS (RGB) > PIXELTAPE    Nats sends pixel data as an RGB framebuffer
#define MODE_FX_TO_PIXELS_W_IR      7   // NATS (FX) > PIXELTAPE     Nats selects one of the build in FX
#define MODE_FX_TO_PIXELS_WO_IR     8   // NATS (FX) > PIXELTAPE     Nats selects one of the build in FX
#define MODE_AUTO_FX_W_IR           9   // Auto loops between build in effects
#define MODE_AUTO_FX_WO_IR          10  // Auto loops between build in effects
#define MODE_WHITE_PIXELS           11  // Puts Pixels to white (emergency)

#define MODE_NR                     12  // the number of modes defined above

/// W_IR = With IR, meaning that IR can interrupt (temp) locally the current running sequence on the pixeltape
/// WO_IR = Without IR, meaning that IR is received and send on NATS, but doesn't interrupt the sequence on the pixeltape

// Reads in the 4 pins that configure the operation mode of the unit and returns them as a single Mode value
inline int getMode() {
	return (digitalRead(MODE2) + 2*digitalRead(MODE1) + 4*digitalRead(MODE4) + 8*digitalRead(MODE3));
}

void printMode(uint mode) {
  switch(mode) {
    case 0: Serial.println("[SYS] Mode 0: External Set"); break;
    case 1: Serial.println("[SYS] Mode 1: DMX In"); break;
    case 2: Serial.println("[SYS] Mode 2: DMX Out"); break;
    case 3: Serial.println("[SYS] Mode 3: DMX > Pixels With IR"); break;
    case 4: Serial.println("[SYS] Mode 4: DMX > Pixels W/O IR"); break;
    case 5: Serial.println("[SYS] Mode 5: RGB > Pixels With IR"); break;
    case 6: Serial.println("[SYS] Mode 6: RGB > Pixels W/O IR"); break;
    case 7: Serial.println("[SYS] Mode 7: FX > Pixels With IR"); break;
    case 8: Serial.println("[SYS] Mode 8: FX > Pixels W/O IR"); break;
    case 9: Serial.println("[SYS] Mode 9: Auto FX With IR"); break;
    case 10: Serial.println("[SYS] Mode 10: Auto FX W/O IR"); break;
    case 11: Serial.println("[SYS] Mode 11: White Mode"); break;
    default: Serial.println("[SYS] Invalid Mode"); 
  }
}