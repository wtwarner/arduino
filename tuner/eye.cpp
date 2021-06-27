#include <math.h>
#include <stdint.h>
#include <Wire.h>
#include "eye.h"
#include "global.h"

// potentiometer values corresponding to min/max length.
// calibrated for specific PWM frequency/duty cycle.
static const uint16_t eye_min = 25, eye_max = 128;
// length of each half in mm
static const float len_min = 3.0, len_max = 17.5;

static void writePot(uint16_t value)
{
  const uint16_t d = (value & 0x3ff) 
    | (0 << 12) // register address 0
    | (0 << 10); // command - write
  digitalWrite(EYE_PIN_POT_CS, 0);
  for (int i = 0; i < 16; i++) {
    digitalWrite(EYE_PIN_POT_SDI, (d >> (15 - i)) & 0x1);
    digitalWrite(EYE_PIN_POT_CLK, 1);
    digitalWrite(EYE_PIN_POT_CLK, 0);
  }
  digitalWrite(EYE_PIN_POT_CS, 1);
}

void setup_eye()
{

  pinMode(EYE_PIN_PWM, OUTPUT);
  pinMode(EYE_PIN_POT_CS, OUTPUT);
  pinMode(EYE_PIN_POT_CLK, OUTPUT);
  pinMode(EYE_PIN_POT_SDI, OUTPUT);

  digitalWrite(EYE_PIN_POT_CS, 1);
  digitalWrite(EYE_PIN_POT_CLK, 0);
  digitalWrite(EYE_PIN_POT_SDI, 0);
  // teensy 3.2 PWM ideal frequency, near 1KHz, for 15-bit resolution.
  // The PWM drives A/C via transistor to generate -22V power supply.
  analogWriteFrequency(EYE_PIN_PWM, 1098.632);
  analogWriteResolution(15);
  analogWrite(EYE_PIN_PWM, 64);
  write_eye(0);
}


//
// the exp function is a curve-fit output of Excel
static byte map_eye(byte data)
{
   float len = data/100.0 * (len_max - len_min) + len_min;
   byte g = map(.0095*expf(len*.267), 0, 1.0, eye_min, eye_max);
   return constrain(g, 0, 128);
}

// 0..100 is open .. close
void write_eye(byte value)
{
    writePot(map_eye(value));
}
