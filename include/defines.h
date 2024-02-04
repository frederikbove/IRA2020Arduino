#define VERSION      9    // This is for HTTP based OTA, end user release versions tracker
/* Version History
* VERSION 1 : original HTTP OTA implementation
* VERSION 2 : intermediate version Daan
* VERSION 3 : final version DAAN
* VERSION 4 : disabled IR for WDT errors on some boards (Koen)
* VERSION 5 : doing some setup logic for EEPROM, fix pixellength 10 issue
* VERSION 6 : fixing and debugging on camp
* VERSION 7 : Fixed OTA
* VERSION 8 : Only try Wifi 5 times before turning on lamps, added rainbow FX
* VERSION 9 : Added RGB2DMX functionality, set OTA to 10min interval
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
#define NATS_SERVER       "51.15.125.58"
#define NATS_ROOT_TOPIC   "area3001"
#define MAX_PIXELS        255       // @TODO this needs to increase, but then memory map needs to be adjusted

#define _IR_ENABLE_DEFAULT_ false     // This has been moved to compiler arguments
#define DECODE_JVC          true