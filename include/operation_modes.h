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

#define MODE_NR                     11  // the number of modes defined above

/// W_IR = With IR, meaning that IR can interrupt (temp) locally the current running sequence on the pixeltape
/// WO_IR = Without IR, meaning that IR is received and send on NATS, but doesn't interrupt the sequence on the pixeltape