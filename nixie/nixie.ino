/*
  Nixie:
  
  3 shift registers
     0..3:
     4:     LED
     8..11    Nixie 1
     12..15   Nixie 0
     16..19   Nixie 3
     20..23   Nixie 2
  */
#include <IPAddress.h>
#include <arduino-timer.h>

const int latchPin = 15;
const int clockPin = 14;
const int dataPin = 20;
const int dimPin = 16; // pwm nixie
const int clockSqwPin = 21;

const int neonPins[] = {2, 3};
const int nixie_dec_pins[] = {9,10,11,12};

const int NUM_LEDS = 24;

byte builtin_led_state = 0;
byte led_state = 0;

byte nixie_digits[4] = {1,2,3,4};

Timer<> timer;
volatile byte interrupt_pending = false;

extern void ds3231_setup(int clockSqwPin, void (*isr)(void));

//  TimeZone library
// https://github.com/JChristensen/Timezone

#include <Timezone.h>

// US Pacific Time Zone
TimeChangeRule myPDT = {"PDT", Second, Sun, Mar, 2, -60*7};    // Daylight time = UTC - 7 hours
TimeChangeRule myPST = {"PST", First, Sun, Nov, 2, -60*8};     // Standard time = UTC - 8 hours
Timezone myTZ(myPDT, myPST);
TimeChangeRule *tcr;        // pointer to the time change rule, use to get TZ abbrev

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(dimPin, OUTPUT); 
  pinMode(neonPins[0], OUTPUT);
  pinMode(neonPins[1], OUTPUT);
  for (int i = 0; i < 4; i ++) {
    pinMode(nixie_dec_pins[i], OUTPUT);
  }

  ds3231_setup(clockSqwPin, onehz_interrupt);
 

  Serial.begin(9600);
  int timeout = 200;
  while (!Serial && --timeout) {
    ; // wait for serial port to connect. Needed for native USB port only.  Wait 2 seconds at most.
    delay(10);
  }

  Serial.println("Starting");
  wifi_setup();

  timer.every(250, toggle_builtin_led);
  timer.every(2000, toggle_led);
  //timer.every(10000, wifi_connect);
  //timer.every(1000, shift_out_nixie);
  timer.every(10, toggle_neon);
  //timer.every(500, update_nixie);
  
 }

void onehz_interrupt()
{
  interrupt_pending = true;
}

bool toggle_builtin_led(void *)
{
   digitalWrite(LED_BUILTIN, builtin_led_state);
   builtin_led_state = 1 - builtin_led_state;

   return true;
}

bool toggle_led(void *)
{
  led_state = 1 - led_state;

  for (int i = 0; i < 4; i ++) {
    digitalWrite(nixie_dec_pins[i], led_state);
  }

  return true;
}

byte shiftData [NUM_LEDS/8] = {0};

bool shift_out_nixie(void *)
{
  digitalWrite(latchPin, LOW);
  for (unsigned int i = 0; i < NUM_LEDS/8; i ++) {
    shiftOut(dataPin, clockPin, MSBFIRST, shiftData[i]);
  }
  digitalWrite(latchPin, HIGH);

  return true;
}

bool update_nixie(void *)
{
  time_t utc = ds3231_get_unixtime();
  time_t local = myTZ.toLocal(utc, &tcr);
  printDateTime(local, tcr -> abbrev);
  nixie_digits[3] = hour(local) / 10;
  nixie_digits[2] = hour(local) % 10;
  nixie_digits[1] = minute(local) / 10;
  nixie_digits[0] = minute(local) % 10;
  nixie_digits[1] = second(local) / 10; // FIXME test
  nixie_digits[0] = second(local) % 10;
//  for (int i = 0; i < 4; i ++) {
//      nixie_digits[i] += 1;
//      nixie_digits[i] %= 10;
//  }
  return true;
}

struct neon_state_t {
  byte x;
  byte cnt;
  byte flop;
  void reset() {
    flop = 0;
    x = 254;
    cnt = 100;
  }
  void toggle() {
    cnt = 100;
    x = 254;
    flop = 1 - flop;
  }
};
neon_state_t neon_state = {254, 200};

bool toggle_neon(void *)
{

  byte g = gamma8(neon_state.x);
  
  analogWrite(neonPins[0], neon_state.flop ? g : 1);
  analogWrite(neonPins[1], neon_state.flop ? 1: g);

  if (--neon_state.cnt == 0) {
    // hold state
  }
  else
    neon_state.x -= 1;

  return true;
}

void pack_shift_data()
{
   memset(shiftData, 0, sizeof(shiftData));
  for (unsigned int i = 0; i < NUM_LEDS; i ++) {
    byte v = 0;
    switch (i) {
        case 4: v = led_state; break;
        
//        case 12: v = 1; break;
//        case 13: v = 1; break;
//        case 14: v = 1; break;
//        case 15: v = 0 /*led_state*/; break;

        default: 
          if (i >= 8) {
              byte digit_i = ((i - 8) / 4) ^ 1; // flip even/odd digits
              byte bit_i = i % 4;
              v = (~nixie_digits[digit_i] >> bit_i) & 0x1;
          }
          else {
             v = 0;
          }
          break;
    }
    shiftData[i/8] |= v << (i%8);
  }
}

void loop() {
  //analogWrite(dimPin, led_state ? 0 : 254);
  digitalWrite(dimPin, 0/*led_state*/);

  if (interrupt_pending) {
    update_nixie(0);
    interrupt_pending = false;
    pack_shift_data();
    shift_out_nixie(0);
    neon_state.toggle();
  }

  delay(1);
  timer.tick();
}
