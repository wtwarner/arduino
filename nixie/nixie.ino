/*
  Nixie.  For Arduino Nano 33 IoT.

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
#include <ArduinoLog.h>
#include <serial-readline.h>

extern bool ds3231_setup(int clockSqwPin, void (*isr)(void));

//
// Pin assignments
//
const int latchPin = 15;
const int clockPin = 14;
const int dataPin = 20;
const int dimPin = 16; // pwm nixie
const int clockSqwPin = 21;
const int neonPins[] = {2, 3};
const int nixie_dec_pins[] = {10, 11, 12, 9}; // nixie decimal point pins

const int NUM_LEDS = 24;

byte builtin_led_state = 0;
byte led_state = 0;

byte nixie_digits[4] = {1, 2, 3, 4};
byte nixie_dec_state[4] = {0, 0, 0, 0};
byte shiftData[NUM_LEDS / 8] = {0};

Timer<> timer;

struct options_t {
public:
    options_t() {
        to_defaults();
    }
    void to_defaults() {
        //nixie_brightness = 192;
        twentyfour_hour = false;
        show_seconds = false;
        neon_gamma = false;
        neon_start = 192;
        neon_off = 1;
        neon_steps = 100;
        neon_stepsize = 1;
        neon_timestep = 10;
        neon_mode = 1;

        antipoison_hour = 5;
        antipoison_min = 13;
        antipoison_duration_ms = 600;
    }
  //byte nixie_brightness;
  bool twentyfour_hour;
  bool show_seconds;
  bool neon_gamma;
  byte neon_start;
  byte neon_off;
  byte neon_steps;
  byte neon_stepsize;
  byte neon_timestep;
  byte neon_mode;

  byte antipoison_hour;
  byte antipoison_min;
  int antipoison_duration_ms;
};

options_t options;

struct nonvolatile_state_t {
  nonvolatile_state_t() {
    init();
  }

  void init()
  {
    memset(unpackedBytes, 0, Size);
    memset(packedBytes, 0, PackedSize);
    st.nixie_brightness = 192;
    st.neon_brightness = 192;
    update_checksum();
  }

  void update_checksum() {
    st.checksum = 255;
    for (int i = 0; i < Size - 1; i++) {
      st.checksum += unpackedBytes[i]; // modulo 8-bit sum
    }
  }

  bool validate_checksum() {
    unpack();
    uint8_t sum = 255;
    for (int i = 0; i < Size - 1; i++) {
      sum += unpackedBytes[i]; // modulo 8-bit sum
    }
    return sum == st.checksum;
  }

  uint8_t *get_packed_bytes() {
    update_checksum();
    pack();
    return packedBytes;
  }

  // the packed fields below represent usable DS3231 fields in Alarm1/2.
  // The DS3231 fields are BCD, so valid values of a 4-bit field is 0..9.
  // Values > 9 could result in undefined behavior, so we restrict ourselves to use 3 bits.
  //
#   define LIST_PACKED_FIELDS(_)                                                 \
  /* packed        unpacked                                                 */ \
  /* byte    bit   byte     bit                                             */ \
  /* offset, lsb,  offset,  lsb, width                                      */ \
  _( 0,      0,    0,       0,   3) /* brightness[2:0] - Alarm 1 seconds    */ \
  _( 0,      4,    0,       3,   2) /* brightness[4:3] - Alarm 1 10 seconds */ \
  _( 1,      0,    0,       5,   3) /* brightness[7:5] - Alarm 1 minutes    */ \
  _( 1,      4,    1,       0,   2) /* neo_brightness[1:0] - Alarm 1 10 minutes */ \
  _( 2,      0,    1,       2,   3) /* neo_brightness[4:2] - Alarm 1 Hour       */ \
  _( 3,      0,    1,       5,   3) /* neo_brightness[7:5] - Alarm 1 Date       */ \
  _( 4,      0,    7,       0,   3) /* checksum[2:0] - Alarm 2 minutes      */ \
  _( 4,      4,    7,       3,   2) /* checksum[4:3] - Alarm 2 10 minutes   */ \
  _( 5,      0,    7,       5,   3) /* checksum[7:5] - Alarm 2 Hours */
  //_( 6,      0,    -,       -,   3) /* n/a - Alarm 2 Date */

  void pack()
  {
    memset(packedBytes, 0, PackedSize);
#       define PACK_FIELD(p_off, p_lsb, up_off, up_lsb, width) \
  packedBytes[p_off] |= ((unpackedBytes[up_off] >> up_lsb) & ((1 << width) - 1)) << p_lsb;

    LIST_PACKED_FIELDS(PACK_FIELD)
#       undef PACK_FIELD
  }

  void unpack()
  {
    memset(unpackedBytes, 0, Size);
#       define UNPACK_FIELD(p_off, p_lsb, up_off, up_lsb, width) \
  unpackedBytes[up_off] |= ((packedBytes[p_off] >> p_lsb) & ((1 << width) - 1)) << up_lsb;

    LIST_PACKED_FIELDS(UNPACK_FIELD)
#       undef UNPACK_FIELD
  }

#   undef LIST_PACKED_FIELDS

  int get_packed_size() const {
    return PackedSize;
  }

  const static int Size = 8;
  union {
    uint8_t unpackedBytes[Size];
    struct {
      uint8_t nixie_brightness; // 0..255
      uint8_t neon_brightness; // 0..255
      uint8_t reserved[Size - 2 - 1];
      uint8_t checksum;
    } st;
  };

  // packed size is 7 bytes, which is the size of DS3231 Alarm1 + Alarm2 registers.
  // However, not all the fields in them are used.
  const static int PackedSize = 7;
  uint8_t packedBytes[PackedSize];
};

nonvolatile_state_t g_nv_options;

void serial_received(char *s)
{
  String str(s);
  cmd_parse(str);
}

SerialLineReader<Stream> serial_reader(Serial, serial_received);

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

struct neon_state_t {
  neon_state_t() {
      reset();
  }
  void reset() {
    flop = 0;
    x = options.neon_start;
    cnt = options.neon_steps;
  }
  void toggle() {
    flop = 1 - flop;
    x = options.neon_start;
    cnt = options.neon_steps;
  }

  byte x;
  byte cnt;
  byte flop;
  Timer<>::Task timer_handle;
};
neon_state_t neon_state;

enum { DISP_STATE_UNSET, DISP_STATE_TIME, DISP_STATE_DATE, DISP_STATE_YEAR, DISP_STATE_ANTIPOISON };
static int display_state = DISP_STATE_UNSET;
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

  Log.begin(LOG_LEVEL_VERBOSE, &Serial);
  Log.trace("Starting\n");

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(dimPin, OUTPUT);
  pinMode(neonPins[0], OUTPUT);
  pinMode(neonPins[1], OUTPUT);
  
  for (int i = 1; i < 4; i++) {
    pinMode(nixie_dec_pins[i], OUTPUT);
   
  }
  pinMode(clockSqwPin, INPUT_PULLUP);

  if (ds3231_setup(clockSqwPin, onehz_interrupt)) {
      display_state = DISP_STATE_TIME;
  }
  
  if (ds3231_getUserBytes(g_nv_options.get_packed_bytes(), g_nv_options.get_packed_size())) {
    if (g_nv_options.validate_checksum()) {
      Log.trace("Get valid user state; nixie b %d, neon b %d\n", g_nv_options.st.nixie_brightness, g_nv_options.st.neon_brightness);
    }
    else {
      Log.trace("Bad user state; re-init\n");
      g_nv_options.init();
      write_nvm();
    }
    options.neon_start = g_nv_options.st.neon_brightness;
  }
  set_nixie_brightness();

  timer.every(250, toggle_builtin_led);
  timer.every(200, check_time);
  timer.every(2000, toggle_led);

  // wifi_setup();
  ble_setup();

  neon_state.reset();
  neon_state.timer_handle = timer.every(options.neon_timestep, toggle_neon);

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

  return true;
}

void shift_out_nixie() {
  digitalWrite(latchPin, LOW);
  for (unsigned int i = 0; i < NUM_LEDS / 8; i++) {
    shiftOut(dataPin, clockPin, MSBFIRST, shiftData[i]);
  }
  digitalWrite(latchPin, HIGH);
  
  for (int i = 1; i < 2; i++) { // FIXME dec_pin[0] is bad
    analogWrite(nixie_dec_pins[i], nixie_dec_state[i] ? g_nv_options.st.nixie_brightness : 0); // 255-nixie_dec_state[i]);
  }
}

bool check_time(void *) {

  time_state.set_utc(ds3231_get_unixtime());

  return true;
}

void update_nixie_unset() {
  nixie_digits[3] = 1;
  nixie_digits[2] = 2;
  nixie_digits[1] = 0;
  nixie_digits[0] = 0;
  // fast flash all decimal points
  for (int i = 0; i < 4; i++) {
    nixie_dec_state[i] = builtin_led_state ? g_nv_options.st.nixie_brightness : 0;
  }
}

void update_nixie_time() {
  static int prev_min = 0;
  if (time_state.local / 60 != prev_min) {
      printDateTime(time_state.local, tcr->abbrev);
      prev_min = time_state.local / 60;
  }
  const byte hour12or24 = options.twentyfour_hour ? hour(time_state.local) : hourFormat12(time_state.local);
  byte hour10 = hour12or24 / 10;
  hour10 = (hour10 > 0) ? hour10 : 15; // blank leading 0
  const byte pm = options.twentyfour_hour ? 0 : isPM(time_state.local);
  nixie_digits[3] = hour10;
  nixie_digits[2] = hour12or24 % 10;
  if (options.show_seconds) {
    nixie_digits[1] = second(time_state.local) / 10;
    nixie_digits[0] = second(time_state.local) % 10;
  } else {
    nixie_digits[1] = minute(time_state.local) / 10;
    nixie_digits[0] = minute(time_state.local) % 10;
  }
  for (int i = 0; i < 4; i++) {
    nixie_dec_state[i] = 0;
  }
  nixie_dec_state[1] = pm;
}

void update_nixie_date() {
  const byte m = month(time_state.local);
  const byte m10 = m / 10;
  const byte m1 = m % 10;
  const byte d = day(time_state.local);
  nixie_digits[3] = (m10 > 0) ? m10 : 0xf; // blank leading 0
  nixie_digits[2] = m1;
  nixie_digits[1] = d / 10;
  nixie_digits[0] = d % 10;

  for (int i = 0; i < 4; i++) {
    nixie_dec_state[i] = g_nv_options.st.nixie_brightness;
  }
}

void update_nixie_year() {
  nixie_digits[3] = year(time_state.local) / 1000;
  nixie_digits[2] = year(time_state.local) % 1000 / 100;
  nixie_digits[1] = year(time_state.local) % 100 / 10;
  nixie_digits[0] = year(time_state.local) % 10;

  for (int i = 0; i < 4; i++) {
    nixie_dec_state[i] = g_nv_options.st.nixie_brightness;
  }
}
static byte disused3[] = {0, 2, 3, 4, 5, 6, 7, 8, 9};
static byte disused2[] = {0, 15};
static byte disused1[] = {6, 7, 8, 9};
static byte disused0[] = {15};
void update_nixie_antipoison() {
  if (options.show_seconds) {
    for (int i = 0; i < 4; i++) {
      nixie_digits[i] = (antipoison_cnt/10 + i) % 10;
    }
  }
  else {
    nixie_digits[3] =
        disused3[antipoison_cnt % (sizeof(disused3) / sizeof(byte))];
    nixie_digits[2] =
        disused2[antipoison_cnt % (sizeof(disused2) / sizeof(byte))];
    nixie_digits[1] =
        disused1[antipoison_cnt % (sizeof(disused1) / sizeof(byte))];
    nixie_digits[0] =
        disused0[antipoison_cnt % (sizeof(disused0) / sizeof(byte))];
  }
  for (int i = 0; i < 4; i++) {
    nixie_dec_state[i] = (antipoison_cnt % 2) ? g_nv_options.st.nixie_brightness : 0;
  }

}

bool toggle_neon(void *) {

  byte g = options.neon_gamma ? gamma8(neon_state.x) : neon_state.x;
  bool count = true;
  switch (options.neon_mode) {
      case 0:
          analogWrite(neonPins[0], neon_state.flop ? g : options.neon_off);
          analogWrite(neonPins[1], neon_state.flop ? options.neon_off : g);
          break;
      case 1:
          analogWrite(neonPins[0], g);
          analogWrite(neonPins[1], g);
          count = false;
          break;
      case 2:
          analogWrite(neonPins[0], neon_state.flop ? g : options.neon_off);
          analogWrite(neonPins[1], neon_state.flop ? g : options.neon_off);
          break;
      default:
          analogWrite(neonPins[0], options.neon_off);
          analogWrite(neonPins[1], options.neon_off);
          count = false;
          break;
  }
  if (count) {
      if (--neon_state.cnt == 0) {
          // hold state
          neon_state.cnt = 1;
      } else
          neon_state.x -= options.neon_stepsize;
  }
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
  byte pwm = /*gamma8*/ (255 - g_nv_options.st.nixie_brightness);
  Serial.print("brightness:");
  Serial.print(g_nv_options.st.nixie_brightness);
  Serial.print(" ");
  Serial.println(pwm);
  analogWrite(dimPin, pwm);
}

void loop() {

  switch (display_state) {
    case DISP_STATE_UNSET:
    update_nixie_unset();
    pack_shift_data();
    shift_out_nixie();
    break;
  case DISP_STATE_TIME:
    if (time_state.new_time) {
      update_nixie_time();
      pack_shift_data();
      shift_out_nixie();
    }
    break;
  case DISP_STATE_DATE:
    if (time_state.new_time) {
      update_nixie_date();
      pack_shift_data();
      shift_out_nixie();
      if (millis() - start_millis > 3000) {
        display_state = DISP_STATE_TIME;
      }
    }
    break;
  case DISP_STATE_YEAR:
    if (time_state.new_time) {
      update_nixie_year();
      pack_shift_data();
      shift_out_nixie();
      if (millis() - start_millis > 3000) {
        display_state = DISP_STATE_TIME;
      }
    }
    break;
  case DISP_STATE_ANTIPOISON:
    update_nixie_antipoison();
    pack_shift_data();
    shift_out_nixie();
    if (millis() - start_millis > 100) {
      if (--antipoison_cnt == 0) {
        display_state = DISP_STATE_TIME;
        
      }
      start_millis = millis();
    }
    break;
  }

  if (time_state.new_time) {
    neon_state.toggle();
    time_state.new_time = false;

    if (((hour(time_state.local) == options.antipoison_hour) || (options.antipoison_hour >= 24)) &&
        ((minute(time_state.local) == options.antipoison_min) || (options.antipoison_min >= 60)) && 
        second(time_state.local) == 0 && display_state != DISP_STATE_ANTIPOISON) {
          display_state = DISP_STATE_ANTIPOISON;
          antipoison_cnt = options.antipoison_duration_ms / 100 + 1;
        }
  }
  delay(1);
  timer.tick();

  String bt_cmd;
  if (ble_loop(bt_cmd)) {
    cmd_parse(bt_cmd);
  }

  serial_reader.poll();
}

void cmd_parse(String &bt_cmd) {
  int v = 0;
  int i;
  if ((i = bt_cmd.indexOf(':')) >= 0) {
    String value_s(bt_cmd.substring(i + 1));
    v = atoi(value_s.c_str());
  }

  time_t new_utc = time_state.utc;
  if (bt_cmd.startsWith("B:")) {
    if (g_nv_options.st.nixie_brightness != v) {
      g_nv_options.st.nixie_brightness = v;
      set_nixie_brightness();
      write_nvm();
    }  
  } else if (bt_cmd.startsWith("NB:")) {
    options.neon_start = v;
    if (g_nv_options.st.neon_brightness != v) {
      g_nv_options.st.neon_brightness = v;
      write_nvm();
    }
  } else if (bt_cmd.startsWith("NM:")) {
    options.neon_mode = v;
  } else if (bt_cmd.startsWith("24:")) {
    options.twentyfour_hour = v;
  } else if (bt_cmd.startsWith("SS:")) {
    options.show_seconds = v;
  } else if (bt_cmd.startsWith("H:") && (v != 0)) {
    new_utc += v * 3600;
  } else if (bt_cmd.startsWith("M:") && (v != 0)) {
    new_utc += v * 60;
  } else if (bt_cmd.startsWith("S:") && (v != 0)) {
    new_utc += v;
  } else if (bt_cmd.startsWith("ZS:") && (v != 0)) {
      // rotate among 3 modes 
      switch (display_state) {
          case DISP_STATE_TIME:
              display_state = DISP_STATE_DATE;
              break;
          case DISP_STATE_DATE:
              display_state = DISP_STATE_YEAR;
              break;
         case DISP_STATE_YEAR:
              display_state = DISP_STATE_ANTIPOISON;
              antipoison_cnt = options.antipoison_duration_ms / 100 + 1;
              break;
          case DISP_STATE_ANTIPOISON:
              display_state = DISP_STATE_TIME;
              break;
      }
      start_millis = millis();
  }
  else if (bt_cmd.startsWith("set-time:")) { // local "ISO 8601" format e.g. 2020-06-25T15:29:37
      time_t local_time = datetime_parse(bt_cmd.substring(bt_cmd.indexOf(':') + 1));
      new_utc = myTZ.toUTC(local_time);
  }
  else if (bt_cmd.startsWith("set-neon:")) {
      char s[100];              // local copy to allow modification by strtok
      strlcpy(s, bt_cmd.c_str(), sizeof(s));
      char *pch = strtok(s + bt_cmd.indexOf(':') + 1, " ");
      if (pch != 0) { options.neon_gamma = atoi(pch); pch = strtok(0, " "); }
      if (pch != 0) { 
        byte v = atoi(pch);
        pch = strtok(0, " "); 
        options.neon_start = v; 
        if (g_nv_options.st.neon_brightness != v) {
          g_nv_options.st.neon_brightness = v; 
        }
      }
      if (pch != 0) { options.neon_steps = atoi(pch); pch = strtok(0, " "); }
      if (pch != 0) { options.neon_stepsize = atoi(pch); pch = strtok(0, " "); }
      if (pch != 0) { options.neon_timestep = atoi(pch); pch = strtok(0, " "); }
      if (pch != 0) { options.neon_off = atoi(pch); pch = strtok(0, " "); }
      if (pch != 0) { options.neon_mode = atoi(pch); pch = strtok(0, " "); }

      timer.cancel(neon_state.timer_handle);
      neon_state.timer_handle = timer.every(options.neon_timestep, toggle_neon);
  }
  else if (bt_cmd.startsWith("set-ap:")) {
      char s[100];              // local copy to allow modification by strtok
      strlcpy(s, bt_cmd.c_str(), sizeof(s));
      char *pch = strtok(s + bt_cmd.indexOf(':') + 1, " ");
      if (pch != 0) { options.antipoison_hour = atoi(pch); pch = strtok(0, " "); }
      if (pch != 0) { options.antipoison_min = atoi(pch); pch = strtok(0, " "); }
      if (pch != 0) { options.antipoison_duration_ms = atoi(pch); pch = strtok(0, " "); }
      Log.trace("set antipoison %d:%d duration %d\n", options.antipoison_hour, options.antipoison_min, options.antipoison_duration_ms);
  }
  if (time_state.utc != new_utc) {
    Serial.print("Set time: ");
    printDateTime(new_utc, tcr->abbrev);
    time_state.set_utc(new_utc);
    ds3231_set(new_utc);
    display_state = DISP_STATE_TIME;
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

void write_nvm() {
  ds3231_setUserBytes(g_nv_options.get_packed_bytes(), g_nv_options.get_packed_size());
}
