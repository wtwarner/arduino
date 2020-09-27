#include "common.h"
const int latchPin = 6;
const int clockPin = 12;
const int dataPin = 9; // LED is 11
const int dimPin = 10;
const int dimPin2 = 14;
const int dimPin3 = 15;

const int NUM_LEDS = 8*5;

struct led_state_t {
  led_state_t(): 
    deadline(0),
    pwm(0xffff),
    state(0)
    {}

  long deadline;
  uint16_t pwm;
  byte state;
};

led_state_t leds[NUM_LEDS];
elapsedMillis shift_timer;
byte shift_phase;

void update_pwm(byte v)
{
   byte g = 255-gamma8(v);
   analogWrite(dimPin, g);
   analogWrite(dimPin2, g);
   analogWrite(dimPin3, g);

}

void shift_setup(shift_state_t &state)
{
  Serial.print("EEPROM read shift pwm: "); Serial.println(state.pwm);

  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(dimPin, OUTPUT);
  pinMode(dimPin2, OUTPUT);
  pinMode(dimPin3, OUTPUT);

  for (unsigned int i = 0; i < NUM_LEDS; i ++) {
    leds[i].state = 1;
  }

  phase = 0;
  update_pwm(state.pwm);

}

void shift_output()
{
  byte shiftData [NUM_LEDS/8] = {0};

  for (unsigned int i = 0; i < NUM_LEDS; i ++) {
    shiftData[(NUM_LEDS-i-1)/8] |= (leds[i].state & leds[i].pwm & 0x1) << (i%8);
  }
  digitalWrite(latchPin, LOW);
  for (unsigned int i = 0; i < NUM_LEDS/8; i ++) {
    shiftOut(dataPin, clockPin, MSBFIRST, shiftData[i]);
  }
  digitalWrite(latchPin, HIGH);
  
}

byte cycle = 0;
byte shift_mode = 1;
void shift_loop(shift_state_t &state)
{

  if (state.mode == 0 && shift_timer > 5) {
      int pwm = (int)sin8(shift_phase) * state.pwm;
      update_pwm(pwm >> 8);
      for (unsigned int i = 0; i < NUM_LEDS; i ++) {  
        //leds[i].state = shift_phase > i*360/(NUM_LEDS-1);
        leds[i].state = i <= cycle;
      }
      shift_phase = (shift_phase + 1) % 360;
      shift_timer = 0;
      if (shift_phase == 0)
        cycle = (cycle + 1) % NUM_LEDS;
      
   
  }
  else if (state.mode == 1 && shift_timer > 5) {
    long time_now = millis();
    for (unsigned int i = 0; i < NUM_LEDS; i ++) {  
        if (leds[i].deadline < time_now) {
          leds[i].state = random(0,12) > 0;
          leds[i].deadline = time_now + (leds[i].state ? random(1000,5000) : random(1,50));
          switch (random(0,5)) {

            case 0: leds[i].pwm = 0xfefe; break;
            case 1: leds[i].pwm = 0xfffe; break;
            case 2: leds[i].pwm = 0xaaaa; break;
            default: leds[i].pwm = 0xffff; break;
          }
        }
      }
      shift_timer = 0;
      update_pwm(state.pwm);
  }
  else if (state.mode == 2 && shift_timer > 5) {
    for (unsigned int i = 0; i < NUM_LEDS; i ++) {  
      leds[i].state = 0;
      leds[i].pwm = 0xffff;
    }
    if (state.test_value < NUM_LEDS) {
      leds[state.test_value].state = 1;
    }
    shift_timer = 0;
    update_pwm(state.pwm);
  }
  else {
    // shift out
    for (unsigned int i = 0; i < NUM_LEDS; i ++) {  
      leds[i].pwm = 0x1 | (leds[i].pwm << 1);
    }
  }
  shift_output();
}
