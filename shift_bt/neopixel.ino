// A basic everyday NeoPixel strip test program.

// NEOPIXEL BEST PRACTICES for most reliable operation:
// - Add 1000 uF CAPACITOR between NeoPixel strip's + and - connections.
// - MINIMIZE WIRING LENGTH between microcontroller board and first pixel.
// - NeoPixel strip's DATA-IN should pass through a 300-500 OHM RESISTOR.
// - AVOID connecting NeoPixels on a LIVE CIRCUIT. If you must, ALWAYS
//   connect GROUND (-) first, then +, then data.
// - When using a 3.3V microcontroller with a 5V-powered NeoPixel strip,
//   a LOGIC-LEVEL CONVERTER on the data line is STRONGLY RECOMMENDED.
// (Skipping these may work OK on your workbench but can fail in the field)

#include "common.h"

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
 #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif
#include <math.h>

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1:
#define LED_PIN    13

// How many NeoPixels are attached to the Arduino?
#define LED_COUNT 11
#define N_PHASE 360
#define PHASE_STEP 40



// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
// Argument 1 = Number of pixels in NeoPixel strip
// Argument 2 = Arduino pin number (most are valid)
// Argument 3 = Pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)

elapsedMillis timer;


// setup() function -- runs once at startup --------------------------------

void neo_setup(neo_state_t &state) {
  // These lines are specifically to support the Adafruit Trinket 5V 16 MHz.
  // Any other board, you can remove this part (but no harm leaving it):
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  clock_prescale_set(clock_div_1);
#endif
  // END of Trinket-specific code.

  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  Serial.print("neo brightness: "); Serial.println(state.brightness);
  strip.setBrightness(gamma8(state.brightness)); 
}

int phase = 0;

const byte neo_pos_map[LED_COUNT] = {3, 2, 4, 1, 5, 0, 6, 10, 7, 9, 8};

void neo_loop(neo_state_t &state) {
  if (timer < 4)
    return;
  timer = 0;
  
  strip.setBrightness(gamma8(state.brightness)); 
 
  for (int i=0; i < LED_COUNT; i++) {

    byte s_blue = sin8((i*PHASE_STEP + phase) % N_PHASE);
    byte s_red = sin8((i*PHASE_STEP + phase + 90) % N_PHASE);

    //Serial.print(phase); Serial.print(", ");
    //Serial.print(c); Serial.print("  ");
    byte red = gamma8(s_red);
    byte blue = gamma8(s_blue);
    strip.setPixelColor(neo_pos_map[i], strip.Color(red, 0, blue));

    // This sends the updated pixel color to the hardware.
    strip.show();
    
  }
  phase = (phase + 1) % N_PHASE;
}
