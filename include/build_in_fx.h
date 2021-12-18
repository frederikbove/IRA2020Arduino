// USES                                             SPEED   XFADE   FGND    BGND
#define FX_PIXEL_LOOP       0                   //  x       -       x       x
#define FX_RDN_PIXEL_LOOP   1                   //  x       -       x       x
#define FX_FGND_BGND_LOOP   2                   //  x       -       x       x
#define FX_FGND_BGND_SWITCH 3                   //  x       -       x       x
#define FX_FIRE2021         4                   //  SPARK   COOL    -       -    

#define FX_NR               5

// MORE FX TO DO: 
// https://www.tweaking4all.nl/hardware/arduino/adruino-led-strip-effecten/#LEDStripEffectMeteorRegen
// https://www.tweaking4all.nl/hardware/arduino/adruino-led-strip-effecten/

int    current_fx_pixel = 0;
int    prev_fx_pixel    = -1;

void fx_pixel_loop()
{
    leds[current_fx_pixel].r = fx_fgnd_r;
    leds[current_fx_pixel].g = fx_fgnd_g;
    leds[current_fx_pixel].b = fx_fgnd_b;

    int previous_pixel;

    if(current_fx_pixel == 0)
        previous_pixel = pixel_length-1;    // from 0 > pixel_length-1
    else    
        previous_pixel = current_fx_pixel-1;

    leds[previous_pixel].r = fx_bgnd_r;
    leds[previous_pixel].g = fx_bgnd_g;
    leds[previous_pixel].b = fx_bgnd_b;

    current_fx_pixel++;
    if(current_fx_pixel >= pixel_length)
        current_fx_pixel = 0;

    FastLED.delay(10 * (fx_speed+1));
}

void fx_rdn_pixel_loop()
{
    current_fx_pixel = random(pixel_length);

    leds[current_fx_pixel].r = fx_fgnd_r;
    leds[current_fx_pixel].g = fx_fgnd_g;
    leds[current_fx_pixel].b = fx_fgnd_b;

    if(prev_fx_pixel >= 0)
    {
        leds[prev_fx_pixel].r = fx_bgnd_r;
        leds[prev_fx_pixel].g = fx_bgnd_g;
        leds[prev_fx_pixel].b = fx_bgnd_b;
    }

    prev_fx_pixel = current_fx_pixel;

    FastLED.delay(10 * (fx_speed+1));
}

void fx_fgnd_bgnd_loop()
{
    if(prev_fx_pixel == -1)  // we use prev_fx_pixel to indicate if we are in the FGND or the BGND iteration
    {
        leds[current_fx_pixel].r = fx_fgnd_r;
        leds[current_fx_pixel].g = fx_fgnd_g;
        leds[current_fx_pixel].b = fx_fgnd_b;

        current_fx_pixel++;
        if(current_fx_pixel >= pixel_length)
        {
            current_fx_pixel = 0;
            prev_fx_pixel = -2;
        }
    }
    else
    {
        leds[current_fx_pixel].r = fx_bgnd_r;
        leds[current_fx_pixel].g = fx_bgnd_g;
        leds[current_fx_pixel].b = fx_bgnd_b;

        current_fx_pixel++;
        if(current_fx_pixel >= pixel_length)
        {
            current_fx_pixel = 0;
            prev_fx_pixel = -1;
        }
    }
    FastLED.delay(10 * (fx_speed+1));
}

void fx_fgnd_bgnd_switch()
{
    if(prev_fx_pixel == -1)  // we use prev_fx_pixel to indicate if we are in the FGND or the BGND iteration
    {
        for(int index = 0; index < pixel_length; index++)
        {
            leds[index].r = fx_fgnd_r;
            leds[index].g = fx_fgnd_g;
            leds[index].b = fx_fgnd_b;
        }
        prev_fx_pixel = -2;
    }
    else
    {
        for(int index = 0; index < pixel_length; index++)
        {
            leds[index].r = fx_bgnd_r;
            leds[index].g = fx_bgnd_g;
            leds[index].b = fx_bgnd_b;
        }
        prev_fx_pixel = -1;
    }
    FastLED.delay(10 * (fx_speed+1));
}

/* Adopted from Fire2012 by Mark Kriegsman, July 2012
   as part of "Five Elements" shown here: http://youtu.be/knWiGsmgycY

   This basic one-dimensional 'fire' simulation works roughly as follows:
   There's a underlying array of 'heat' cells, that model the temperature
   at each point along the line.  Every cycle through the simulation, 
   four steps are performed:
    1) All cells cool down a little bit, losing heat to the air
    2) The heat from each cell drifts 'up' and diffuses a little
    3) Sometimes randomly new 'sparks' of heat are added at the bottom
    4) The heat from each cell is rendered as a color into the leds array
       The heat-to-color mapping uses a black-body radiation approximation.
 
   Temperature is in arbitrary units from 0 (cold black) to 255 (white hot).

   This simulation scales it self a bit depending on NUM_LEDS; it should look
   "OK" on anywhere from 20 to 100 LEDs without too much tweaking. 

   I recommend running this simulation at anywhere from 30-100 frames per second,
   meaning an interframe delay of about 10-35 milliseconds.
  
   Looks best on a high-density LED setup (60+ pixels/meter).

   There are two main parameters you can play with to control the look and
   feel of your fire: COOLING (used in step 1 above), and SPARKING (used
   in step 3 above).

   COOLING: How much does the air cool as it rises?
   Less cooling = taller flames.  More cooling = shorter flames.
   Default 50, suggested range 20-100 
#define COOLING  55

   SPARKING: What chance (out of 255) is there that a new spark will be lit?
   Higher chance = more roaring fire.  Lower chance = more flickery fire.
   Default 120, suggested range 50-200.
#define SPARKING 120
*/
void fire2021()
{
  // Array of temperature readings at each simulation cell
  static uint8_t heat[MAX_PIXELS];

  uint8_t sparking = fx_speed;
  // Limit range @TODO: scale range!
  if(sparking < 50)
    sparking = 50;
  if(sparking > 200)
    sparking = 200;

  uint8_t cool = fx_xfade;
  if(cool < 20)
    cool = 20;
   if(cool > 100)
    cool = 100;

  // Step 1.  Cool down every cell a little
    for( int i = 0; i < pixel_length; i++) {
      heat[i] = qsub8( heat[i],  random8(0, ((cool * 10) / pixel_length) + 2));
    }
  
    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for( int k= pixel_length - 1; k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }
    
    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( random8() < sparking ) {
      int y = random8(7);
      heat[y] = qadd8( heat[y], random8(160,255) );
    }

    // Step 4.  Map from heat cells to LED colors
    for( int j = 0; j < pixel_length; j++) {
      CRGB color = HeatColor( heat[j]);
      leds[j] = color;
    }

    FastLED.delay(16);      // should give us about 60fps
}

void process_build_in_fx()
{
    switch (fx_select)
    {
    case FX_PIXEL_LOOP:
        fx_pixel_loop();
        break;

    case FX_RDN_PIXEL_LOOP:
        fx_rdn_pixel_loop();
        break;

    case FX_FGND_BGND_LOOP:
        fx_fgnd_bgnd_loop();
        break;

    case FX_FGND_BGND_SWITCH:
        fx_fgnd_bgnd_switch();
        break;
    
    case FX_FIRE2021:
        fire2021();
        break;

    default:
        Serial.println("[PIX] Invalid FX Set"); 
        break;
    }

}