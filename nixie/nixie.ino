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
#include <TimeLib.h>
#include <Timezone_Generic.h>
#include <arduino-timer.h>

extern void ds3231_setup(int clockSqwPin, void (*isr)(void));

//
// Pin assignments
//
const int latchPin = 15;
const int clockPin = 14;
const int dataPin = 20;
const int dimPin = 16; // pwm nixie
const int clockSqwPin = 21;
const int neonPins[] = {2, 3};
const int nixie_dec_pins[] = {9, 10, 11, 12};

const int NUM_LEDS = 24;

byte builtin_led_state = 0;
byte led_state = 0;

byte nixie_digits[4] = {1, 2, 3, 4};
byte shiftData[NUM_LEDS / 8] = {0};

Timer<> timer;
bool new_time = false;
time_t utc = 0;

struct options_t {
public:
  options_t()
      : nixie_brightness(255), twentyfour_hour(false), show_seconds(false) {}
  byte nixie_brightness;
  bool twentyfour_hour;
  bool show_seconds;
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

enum { DISP_STATE_TIME, DISP_STATE_DATE, DISP_STATE_ANTIPOISON };
static int display_state = DISP_STATE_TIME;
static int antipoison_cnt = 0;
static unsigned long start_millis = 0;

void setup() {

  Serial.begin(9600);
  int timeout = 200;
  while (!Serial && --timeout) {
    ; // wait for serial port to connect. Needed for native USB port only.  Wait
      // 2 seconds at most.
    delay(10);
  }

  Serial.println("Starting");

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(dimPin, OUTPUT);
  pinMode(neonPins[0], OUTPUT);
  pinMode(neonPins[1], OUTPUT);
  for (int i = 0; i < 4; i++) {
    pinMode(nixie_dec_pins[i], OUTPUT);
  }
  pinMode(clockSqwPin, INPUT_PULLUP);

  ds3231_setup(clockSqwPin, onehz_interrupt);

  timer.every(250, toggle_builtin_led);
  timer.every(200, check_time);
  timer.every(2000, toggle_led);
  timer.every(10, toggle_neon);

  // wifi_setup();
  ble_setup();
}

void onehz_interrupt() {
  //  interrupt_pending = true;
}

bool toggle_builtin_led(void *) {
  digitalWrite(LED_BUILTIN, builtin_led_state);
  builtin_led_state = 1 - builtin_led_state;

  return true;
}

bool toggle_led(void *) {
  led_state = 1 - led_state;

  for (int i = 0; i < 4; i++) {
    digitalWrite(nixie_dec_pins[i], led_state); // fixme
  }

  return true;
}

bool shift_out_nixie(void *) {
  digitalWrite(latchPin, LOW);
  for (unsigned int i = 0; i < NUM_LEDS / 8; i++) {
    shiftOut(dataPin, clockPin, MSBFIRST, shiftData[i]);
  }
  digitalWrite(latchPin, HIGH);

  return true;
}

bool check_time(void *) {
  static time_t previous_utc = 0;
  utc = ds3231_get_unixtime();
  new_time = previous_utc != utc;
  previous_utc = utc;

  return true;
}

void update_nixie_time() {
  time_t local = myTZ.toLocal(utc, &tcr);
  printDateTime(local, tcr->abbrev);
  const byte hour24 = hour(local);
  const byte hour12or24 = options.twentyfour_hour ? hour24 : (hour24 % 12);
  byte hour10 = hour12or24 / 10;
  hour10 = (hour10 > 0) ? hour10 : 15; // blank leading 0
  byte pm = options.twentyfour_hour ? 0 : (hour24 >= 12);
  nixie_digits[3] = hour10;
  nixie_digits[2] = hour12or24 % 10;
  if (options.show_seconds) {
    nixie_digits[1] = second(local) / 10;
    nixie_digits[0] = second(local) % 10;
  } else {
    nixie_digits[1] = minute(local) / 10;
    nixie_digits[0] = minute(local) % 10;
  }
}

void update_nixie_date() {
  time_t local = myTZ.toLocal(utc, &tcr);
  const byte m = month(local);
  const byte m10 = m / 10;
  const byte m1 = m % 10;
  const byte d = day(local);
  nixie_digits[3] = (m10 > 0) ? m10 : 0xf; // blank leading 0
  nixie_digits[2] = m1;
  nixie_digits[1] = d / 10;
  nixie_digits[0] = d % 10;
}

static byte disused3[] = {0, 2, 3, 4, 5, 6, 7, 8, 9};
static byte disused2[] = {0, 15};
static byte disused1[] = {6, 7, 8, 9};
static byte disused0[] = {15};
void update_nixie_antipoison() {
  nixie_digits[3] =
      disused3[antipoison_cnt % (sizeof(disused3) / sizeof(byte))];
  nixie_digits[2] =
      disused2[antipoison_cnt % (sizeof(disused2) / sizeof(byte))];
  nixie_digits[1] =
      disused1[antipoison_cnt % (sizeof(disused1) / sizeof(byte))];
  nixie_digits[0] =
      disused0[antipoison_cnt % (sizeof(disused0) / sizeof(byte))];
  antipoison_cnt++;
}

bool toggle_neon(void *) {

  byte g = gamma8(neon_state.x);

  analogWrite(neonPins[0], neon_state.flop ? g : 1);
  analogWrite(neonPins[1], neon_state.flop ? 1 : g);

  if (--neon_state.cnt == 0) {
    // hold state
    neon_state.cnt = 1;
  } else
    neon_state.x -= 1;

  return true;
}

void pack_shift_data() {
  memset(shiftData, 0, sizeof(shiftData));
  for (unsigned int i = 0; i < NUM_LEDS; i++) {
    byte v = 0;
    switch (i) {
    case 4:
      v = led_state;
      break;

    default:
      if (i >= 8) {
        byte digit_i = ((i - 8) / 4) ^ 1; // flip even/odd digits
        byte bit_i = i % 4;
        v = (~nixie_digits[digit_i] >> bit_i) & 0x1;
      } else {
        v = 0;
      }
      break;
    }
    shiftData[i / 8] |= v << (i % 8);
  }
}

void set_nixie_brightness() {
  byte pwm = /*gamma8*/ (255 - options.nixie_brightness);
  Serial.print("brightness:");
  Serial.print(options.nixie_brightness);
  Serial.print(" ");
  Serial.println(pwm);
  analogWrite(dimPin, pwm);
}

void loop() {

  switch (display_state) {
  case DISP_STATE_TIME:
    if (new_time) {
      update_nixie_time();
      pack_shift_data();
      shift_out_nixie(0);
    }
    break;
  case DISP_STATE_DATE:
    if (new_time) {
      update_nixie_date();
      pack_shift_data();
      shift_out_nixie(0);
      if (millis() - start_millis > 3000) {
        display_state = DISP_STATE_TIME;
      }
    }
    break;
  case DISP_STATE_ANTIPOISON:
    update_nixie_antipoison();
    pack_shift_data();
    shift_out_nixie(0);
    if (millis() - start_millis > 100) {
      if (antipoison_cnt++ > 25) {
        display_state = DISP_STATE_TIME;
        antipoison_cnt = 0;
      }
      start_millis = millis();
    }
    break;
  }

  if (new_time) {
    neon_state.toggle();
    new_time = false;
  }
  delay(1);
  timer.tick();

  String bt_cmd;
  if (ble_loop(bt_cmd)) {
    cmd_parse(bt_cmd);
  }
}

void cmd_parse(String &bt_cmd) {
  int v = 0;
  int i;
  if ((i = bt_cmd.indexOf(':')) >= 0) {
    String value_s(bt_cmd.substring(i + 1));
    v = atoi(value_s.c_str());
  }

  time_t new_utc = utc;
  if (bt_cmd.startsWith("B:")) {
    options.nixie_brightness = v;
    set_nixie_brightness();
  } else if (bt_cmd.startsWith("24:")) {
    options.twentyfour_hour = v;
  } else if (bt_cmd.startsWith("SS:")) {
    options.show_seconds = v;
    Serial.print("show seconds: ");
    Serial.println(v);
  } else if (bt_cmd.startsWith("H:") && (v != 0)) {
    new_utc = utc + v * 3600;
  } else if (bt_cmd.startsWith("M:") && (v != 0)) {
    new_utc = utc + v * 60;
  } else if (bt_cmd.startsWith("S:") && (v != 0)) {
    new_utc = utc + v;
  } else if (bt_cmd.startsWith("ZS:") && (v != 0)) {
      switch (display_state) {
          case DISP_STATE_TIME:
              display_state = DISP_STATE_DATE;
              break;
          case DISP_STATE_DATE:
              display_state = DISP_STATE_ANTIPOISON;
              antipoison_cnt = 0;
              break;
          case DISP_STATE_ANTIPOISON:
              display_state = DISP_STATE_TIME;
              break;
      }
      start_millis = millis();
  }
#if 0
  } else if (bt_cmd.startsWith("ZS:") && (v != 0)) {
    // zero seconds
    tmElements_t tm;
    breakTime(utc, tm);
    tm.Second = 0;
    new_utc = makeTime(tm);
  }
#endif
  if (utc != new_utc) {
    Serial.print("Set time: ");
    printDateTime(new_utc, tcr->abbrev);
    ds3231_set(new_utc);
  }
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
