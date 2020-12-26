//
// Teleregister
// for Arduino Nano (5V)

#include <TimeLib.h>
#include <Timezone_Generic.h>
#include <arduino-timer.h>
#include <Arduino.h>
#include <ArduinoLog.h>
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>
#include "ir.h"

extern bool ds3231_setup();

const byte NUM_TR_DIGITS = 4;
const int MIN_DELAY = 120;

const byte tr_blank_pin[] = {2, 11, 9, 7};
const byte tr_nonblank_pin[] = {12, 10, 8, 4};
const byte status_pin[] = {17, 16, 15, 14};
const byte neo_pin = 13;
//#define IRMP_INPUT_PIN 3 in ir.ino
const byte colon_led_pins[2] = { 5, 6 };

Timer<> timer;

struct options_t {
public:
    options_t() {
        to_defaults();
    }
    void to_defaults() {
        sentinel1 = 0xba;
        neo_brightness = 160;
        neo_color = (255ull << 16) | (147ull << 8) | 41ull; // warm white
        twentyfour_hour = false;
        show_seconds = false;
        sentinel2 = 0xbe;
    }
    void validate() {
        if (sentinel1 != 0xba || sentinel2 != 0xbe) {
            to_defaults();
        }
    }
  byte sentinel1;
  short neo_brightness;
  uint32_t neo_color;
  bool twentyfour_hour;
  bool show_seconds;
  byte sentinel2;
    
};

options_t options;

//
// TimeZone
//
TimeChangeRule myPDT = {"PDT", Second, Sun, Mar,
                        2,     -60 * 7}; // Daylight time = UTC - 7 hours
TimeChangeRule myPST = {"PST", First, Sun,
                        Nov,   2,     -60 * 8}; // Standard time = UTC - 8 hours
Timezone myTZ(myPDT, myPST);
TimeChangeRule *tcr; // pointer to the time change rule, use to get TZ abbrev

struct time_state_t {
  time_state_t():
    new_time(false ),
    utc(0),
    local(0)
  {}
  void set_utc(time_t new_utc) {
    if (new_utc != utc) {
      new_time = true;
      utc = new_utc; 
      local = myTZ.toLocal(utc, &tcr);
    }
  }
  bool new_time;
  time_t utc;
  time_t local;
};
time_state_t time_state;

enum { DISP_STATE_UNSET, DISP_STATE_TIME, DISP_STATE_DATE, DISP_STATE_YEAR};
static int display_state = DISP_STATE_UNSET;
static unsigned long start_millis = 0;

static const byte NUM_NEOPIXELS = 4;
Adafruit_NeoPixel neo_strip(NUM_NEOPIXELS, neo_pin, NEO_GRB + NEO_KHZ800);

class tr_digit_t {
public:
    tr_digit_t():
        digit(DIG_BLANK),
        deadline_millis(0),
        prev_millis(0),
        name(0),
	unknown(true)
    {}

     void begin(byte tr_nonblank_pin_, byte tr_blank_pin_, byte status_pin_, byte nm_) {
        tr_nonblank_pin = tr_nonblank_pin_;
        tr_blank_pin = tr_blank_pin_;
        status_pin = status_pin_;
        name = nm_;

        pinMode(tr_nonblank_pin, OUTPUT);
        pinMode(tr_blank_pin, OUTPUT);
        pinMode(status_pin, INPUT);
        digitalWrite(tr_nonblank_pin, 0);
        digitalWrite(tr_blank_pin, 0);
    }

    // displayed values, in order of rotation of cylindar
    enum digit_t { DIG_BLANK,
                   DIG_1, DIG_2, DIG_3, DIG_4, DIG_5,
                   DIG_6, DIG_7, DIG_8, DIG_9, DIG_0 };
    // indexed by digit_t; maps to actual value
    const short digit_to_num[11] = {-1, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0};
    // indexed by actual value 0..9, maps to digit_t
    static const digit_t num_to_digit[10];

    // returns false if busy
    bool set(digit_t target_) {
        if (act_state == ACT_IDLE) {
	  if (unknown || digit != target_) {
            target_digit = target_;
            act_state = ACT_ASSERTED;
            actuate(1);
            Log.trace("%d: start, to ASSERT\n", name);
            return true;
	  }
	}
        return false;
    }
    bool is_idle() {
        return act_state == ACT_IDLE;
    }
    // returns false if busy
    bool loop() {
        unsigned long now_millis = millis();
        if (now_millis < prev_millis) { // rollover
            deadline_millis  = now_millis + MIN_DELAY;
        }
        prev_millis = now_millis;

        const bool timeout = (millis() >= deadline_millis);

        switch (act_state) {
            case ACT_IDLE:
                return true;
            case ACT_ASSERTED:
                if (timeout) {
                    act_state = ACT_DEASSERTED;
                    actuate(0);
                    Log.trace("%d: to DEASSERT\n", name);
                }
                break;
            case ACT_DEASSERTED:
                if (timeout) {
                    digit = static_cast<digit_t>((static_cast<int>(digit) + 1) % 11);
                    if (digit == DIG_BLANK) Log.trace("%d: deassert, on (blank)\n", name);
                    else Log.trace("%d: deassert, on %d\n", name, digit_to_num[digit]);

                    if (!unknown && digit == target_digit) {
                        act_state = ACT_IDLE;
                        Log.trace("%d: done, to IDLE\n", name);
                    }
                    else {
                        act_state = ACT_ASSERTED;
                        actuate(1);
                        Log.trace("%d: not done, target %d, back to ASSERT\n", name, digit_to_num[target_digit]);
                    }
                }
                break;
        }

        return false;
    };
    
private:
    digit_t digit;
    byte tr_nonblank_pin;          // pin to actuate when on non-blank position
    byte tr_blank_pin;    // pin to actuate when on blank position
    byte status_pin;      // pin to query whether on blank position
    enum actuate_state_t {
        ACT_IDLE, ACT_ASSERTED, ACT_DEASSERTED
    };
    actuate_state_t act_state;
    unsigned long deadline_millis;
    unsigned long prev_millis;
    digit_t target_digit;
    byte name;                  // for debug messages
    bool unknown;

  bool check_blank() {
    const byte on_blank = digitalRead(status_pin);
    if (on_blank) {
      digit = DIG_BLANK;
      unknown = false;
    }
    return on_blank;
  }
    void actuate(byte on) {
        const byte on_blank = on ? check_blank() : 0; // for off, don't care about blank state
        digitalWrite(on_blank ? tr_blank_pin : tr_nonblank_pin, on);
        // in 'off', conservatively deassert both pins
        if (!on) {
            digitalWrite(on_blank ? tr_nonblank_pin : tr_blank_pin, 0);
        }
        Log.verbose("%d: act: %d %s\n", name, on, on_blank ? " (blank)" : "");

        prev_millis = millis();
        if (__builtin_uaddl_overflow(prev_millis, MIN_DELAY, &deadline_millis)) {   // handle rollover of addition
            delay(MIN_DELAY);
            prev_millis = millis();
            deadline_millis = prev_millis + MIN_DELAY;
        }
    }
};

const tr_digit_t::digit_t tr_digit_t::num_to_digit[10] = {DIG_0, DIG_1, DIG_2, DIG_3, DIG_4, DIG_5, DIG_6, DIG_7, DIG_8, DIG_9};

tr_digit_t tr_digits[NUM_TR_DIGITS];

bool check_time(void *) {

    time_state.set_utc(ds3231_get_unixtime());

  return true;
}

void update_tr_unset() {
  for (int i = 0; i < NUM_TR_DIGITS; i++) {
      tr_digits[i].set(tr_digit_t::DIG_BLANK);
    }
  // fast flash all decimal points  FIXME
}

void update_tr_time() {
    tr_digit_t::digit_t digs[NUM_TR_DIGITS];
    printDateTime(time_state.local, tcr->abbrev);
    const byte hour12or24 = options.twentyfour_hour ? hour(time_state.local) : hourFormat12(time_state.local);
    const byte hour10 = hour12or24 / 10;
    //const byte pm = options.twentyfour_hour ? 0 : isPM(time_state.local);
    digs[3] = (hour10 > 0) ? tr_digit_t::num_to_digit[hour10] : tr_digit_t::DIG_BLANK;  // blank leading 0
    digs[2] = tr_digit_t::num_to_digit[hour12or24 % 10];
    if (options.show_seconds) {
        digs[1] = tr_digit_t::num_to_digit[second(time_state.local) / 10];
        digs[0] = tr_digit_t::num_to_digit[second(time_state.local) % 10];
    } else {
        digs[1] = tr_digit_t::num_to_digit[minute(time_state.local) / 10];
        digs[0] = tr_digit_t::num_to_digit[minute(time_state.local) % 10];
    }
    for (int i = 0; i < NUM_TR_DIGITS; i++) {
        tr_digits[i].set(digs[i]);
    }
    // FIXME set AM/PM?
}

void update_tr_date() {
    tr_digit_t::digit_t digs[NUM_TR_DIGITS];
    const byte m = month(time_state.local);
    const byte m10 = m / 10;
    const byte m1 = m % 10;
    const byte d = day(time_state.local);
    digs[3] = (m10 > 0) ? tr_digit_t::num_to_digit[m10] : tr_digit_t::DIG_BLANK; // blank leading 0
    digs[2] = tr_digit_t::num_to_digit[m1];
    digs[1] = tr_digit_t::num_to_digit[d / 10];
    digs[0] = tr_digit_t::num_to_digit[d % 10];
    for (int i = 0; i < NUM_TR_DIGITS; i++) {
        tr_digits[i].set(digs[i]);
    }
}

void update_tr_year() {
  tr_digit_t::digit_t digs[NUM_TR_DIGITS];
  digs[3] = tr_digit_t::num_to_digit[year(time_state.local) / 1000];
  digs[2] = tr_digit_t::num_to_digit[year(time_state.local) % 1000 / 100];
  digs[1] = tr_digit_t::num_to_digit[year(time_state.local) % 100 / 10];
  digs[0] = tr_digit_t::num_to_digit[year(time_state.local) % 10];
  for (int i = 0; i < NUM_TR_DIGITS; i++) {
      tr_digits[i].set(digs[i]);
  }

}

void neo_set_color() {
  for (int i=0; i < NUM_NEOPIXELS; i++) {
    neo_strip.setPixelColor(i, options.neo_color);
  }
  neo_strip.setBrightness(options.neo_brightness);
  neo_strip.show();
}

void setup() {
  neo_strip.begin();
  neo_set_color();
  ir_setup();
  
  Serial.begin(9600);
  int timeout = 200;
  while (!Serial && --timeout) {
    // wait for serial port to connect. Needed for native USB port only.  Wait
    // 2 seconds at most.
    delay(10);
  }

  Log.begin(LOG_LEVEL_VERBOSE, &Serial);
  Log.trace("Starting\n");
  EEPROM.get(0, options);
  options.validate();
  EEPROM.put(0, options);
  
  for (int i = 0; i < NUM_TR_DIGITS; i ++) {
      tr_digits[i].begin(tr_nonblank_pin[i], tr_blank_pin[i], status_pin[i], i);
  }
  
  for (int i = 0; i < 2; i ++) {
      pinMode(colon_led_pins[i], OUTPUT);
  }
  if (ds3231_setup()) {
      display_state = DISP_STATE_TIME;
  }
  //ds3231_set(myTZ.toUTC(datetime_compile())); // initialize 1st time

  timer.every(500, toggle_colon_led);
  timer.every(200, check_time);
}

bool toggle_colon_led(void *) {
    byte state = digitalRead(colon_led_pins[0]);
    digitalWrite(colon_led_pins[0], 1 - state);
    digitalWrite(colon_led_pins[1], state);

  return true;
}


void loop() {
  bool idle = true;
  for (int i = 0; i < NUM_TR_DIGITS; i++) {
      tr_digits[i].loop();
      idle = idle && tr_digits[i].is_idle();
  }
  switch (display_state) {
      case DISP_STATE_UNSET:
          if (idle) {
              update_tr_unset();
          }
          break;
      case DISP_STATE_TIME:
          if (idle && time_state.new_time) {
              update_tr_time();
          }
          break;
      case DISP_STATE_DATE:
          if (idle && time_state.new_time) {
              update_tr_date();
              if (millis() - start_millis > 3000) {
                  display_state = DISP_STATE_TIME;
              }
          }
          break;
      case DISP_STATE_YEAR:
          if (idle && time_state.new_time) {
              update_tr_year();
              if (millis() - start_millis > 3000) {
                  display_state = DISP_STATE_TIME;
              }
          }
          break;
  }

  if (time_state.new_time) {
    time_state.new_time = false;
  }

  ir_cmd_t ir_cmd;
  bool ir_rep;
  if (ir_check(ir_cmd, ir_rep)) {
      switch (ir_cmd) {
          case IR_FF:
              if (!ir_rep)
                  options.show_seconds = 1 - options.show_seconds;
              break;
          case IR_FUNCSTOP:
              if (ir_rep) {
                  ; // nop
              }
              else if (display_state == DISP_STATE_TIME) {
                  display_state = DISP_STATE_DATE;
                  start_millis = millis();
              }
              else if (display_state == DISP_STATE_DATE) {
                  display_state = DISP_STATE_YEAR;
                  start_millis = millis();
              }
              else if (display_state == DISP_STATE_YEAR) {
                  display_state = DISP_STATE_TIME;
                  start_millis = millis();
              }
              break;

          case IR_VOLUP:
              options.neo_brightness = min(255, options.neo_brightness + 1);
              neo_set_color();
              break;
          case IR_VOLDOWN:
              options.neo_brightness = max(0, options.neo_brightness - 1);
              neo_set_color();
              break;
      }
      EEPROM.put(0, options);
  }

  delay(1);
  timer.tick();
}

void printDateTime(time_t t, const char *tz) {
  char buf[32];

  char m[4]; // temporary storage for month string (DateStrings.cpp uses shared
             // buffer)
  strcpy(m, monthShortStr(month(t)));
  sprintf(buf, "%.2d:%.2d:%.2d %s %.2d %s %d %s", hour(t), minute(t), second(t),
          dayShortStr(weekday(t)), day(t), m, year(t), tz);
  Serial.println(buf);
}
