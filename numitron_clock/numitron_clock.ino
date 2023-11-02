//
// Teensy 3.2
//

#include <string>
#include <vector>
#include <elapsedMillis.h>
#include "LiquidCrystal.h"
#include <TimeLib.h>
#include <Timezone_Generic.h>
#include <ArduinoLog.h>
#include <RTClib.h>

const byte PIN_DAC_CLK = 10;
const byte PIN_DAC_DATA = 11;
const byte PIN_DAC_LOAD = 12;

const byte PIN_TOUCH[] = {22, 23, 16, 15};

const byte PIN_LCD_RS = 0;
const byte PIN_LCD_EN = 1;
const byte PIN_LCD_D4 = 2;
const byte PIN_LCD_D5 = 3;
const byte PIN_LCD_D6 = 4;
const byte PIN_LCD_D7 = 5;
const byte PIN_LCD_BL_PWM = 21;

const byte PIN_BULB_PWM = 20;

const byte PIN_TPIC_SCLK = 6;
const byte PIN_TPIC_RCLK = 7;
const byte PIN_TPIC_SDI = 8;
const byte PIN_TPIC_OE_ = 9;

// DS3231 RTC uses i2c: SDA = 18, SCL = 19
const byte PIN_RTC_SQW = 14;

//
// LCD
//
LiquidCrystal lcd(PIN_LCD_RS, PIN_LCD_EN, PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7);

//
// TimeZone
//
TimeChangeRule myPDT = {"PDT", Second, Sun, Mar,
                        2,     -60 * 7
                       }; // Daylight time = UTC - 7 hours
TimeChangeRule myPST = {"PST", First, Sun,
                        Nov,   2,     -60 * 8
                       }; // Standard time = UTC - 8 hours
Timezone myTZ(myPDT, myPST);

//
// buttons
//
elapsedMillis buttonTimer;
const int button_threshold[4] = {2000, 2000, 2000, 2000};

//
// non-volatile state
//
struct nonvolatile_state_t {
  nonvolatile_state_t() {
    init();
  }

  void init()
  {
    memset(unpackedBytes, 0, Size);
    memset(packedBytes, 0, PackedSize);
    st.numi_brightness = 192;
    st.colon_brightness = 192;
    st.lcd_brightness = 192;
    update_checksum();
  }

  void update_checksum() {
    st.checksum = 255;
    for (int i = 0; i < Size - 1; i++) {
      st.checksum += unpackedBytes[i]; // modulo 8-bit sum
    }
  }

  bool validate_checksum() {
    Log.trace("ds3231 packed %x %x %x %x %x %x %x\n",
	      packedBytes[0],
	      packedBytes[1],
	      packedBytes[2],
	      packedBytes[3],
	      packedBytes[4],
	      packedBytes[5],
	      packedBytes[6]);
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
  _( 0,      0,    0,       0,   3) /* numi_brightness[2:0] - Alarm 1 seconds    */ \
  _( 0,      4,    0,       3,   2) /* numi_brightness[4:3] - Alarm 1 10 seconds */ \
  _( 1,      0,    0,       5,   3) /* numi_brightness[7:5] - Alarm 1 minutes    */ \
  _( 1,      4,    1,       3,   2) /* lcd_brightness[4:3] - Alarm 1 10 minutes */ \
  _( 2,      0,    1,       5,   3) /* lcd_brightness[7:5] - Alarm 1 Hour       */ \
  _( 3,      0,    2,       2,   3) /* colon_brightness[4:2] - Alarm 1 Date       */ \
  _( 4,      0,    2,       5,   3) /* colon_brightness[7:5] - Alarm 2 minutes      */ \
  _( 4,      4,    7,       0,   2) /* checksum[1:0] - Alarm 2 10 minutes   */ \
  _( 5,      0,    7,       2,   3) /* checksum[4:2] - Alarm 2 Hours */ \
  _( 6,      0,    7,       5,   3) /* checksum[7:5] - Alarm 2 Date */

  void pack()
  {
    memset(packedBytes, 0, PackedSize);
#   define PACK_FIELD(p_off, p_lsb, up_off, up_lsb, width) \
      packedBytes[p_off] |= ((unpackedBytes[up_off] >> up_lsb) & ((1 << width) - 1)) << p_lsb;

    LIST_PACKED_FIELDS(PACK_FIELD)
#   undef PACK_FIELD
  }

  void unpack()
  {
    memset(unpackedBytes, 0, Size);
#   define UNPACK_FIELD(p_off, p_lsb, up_off, up_lsb, width) \
      unpackedBytes[up_off] |= ((packedBytes[p_off] >> p_lsb) & ((1 << width) - 1)) << up_lsb;

    LIST_PACKED_FIELDS(UNPACK_FIELD)
#   undef UNPACK_FIELD

    // postprocess - MSB replicate to 8 bits
      //    st.colon_brightness |= (st.colon_brightness >> 6);
      //    st.lcd_brightness |= (st.lcd_brightness >> 5);
    
  }

#   undef LIST_PACKED_FIELDS

  int get_packed_size() const {
    return PackedSize;
  }

  const static int Size = 8;
  union {
    uint8_t unpackedBytes[Size];
    struct {
      uint8_t numi_brightness;  // 0..255, 8 bits
      uint8_t colon_brightness; // 0..255, 6 MSB bits
      uint8_t lcd_brightness;  // 0..255, 5 MSB bits
      uint8_t reserved[Size - 3 - 1];
      uint8_t checksum;
    } st;
  };

  // packed size is 7 bytes, which is the size of DS3231 Alarm1 + Alarm2 registers.
  // However, not all the fields in them are used.
  const static int PackedSize = 7;
  uint8_t packedBytes[PackedSize];
};

nonvolatile_state_t g_nv_options;


//
// global state
//
struct time_state_t {
  time_state_t():
    new_time(false),
    utc(0),
    local(0),
    tcr(0)
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
  TimeChangeRule *tcr; // pointer to the time change rule, use to get TZ abbrev
};
time_state_t time_state;

elapsedMillis clkTimer;

//
// Numitron
//
//   ---5--
//   |    |
//   6    4
//   ---9--
//   |    |
//   8    3
//   ---7--
//          *2
//

elapsedMillis numiTimer;

enum { DIG_0=0, DIG_1, DIG_2, DIG_3, DIG_4, DIG_5, DIG_6, DIG_7, DIG_8, DIG_9, DIG_DASH = 10 };
byte numi_digits[4] = { DIG_DASH, DIG_DASH, DIG_DASH, DIG_DASH };
byte numi_dec_points[4] = {0,0,0,0};

enum { DISP_STATE_UNSET, DISP_STATE_TIME, DISP_STATE_DATE, DISP_STATE_YEAR, DISP_STATE_TEST };
byte display_state = DISP_STATE_UNSET;

#define S(n) (1<<n)

static const uint16_t numi_digit_to_segments[11] = {
    S(3)|S(4)|S(5)|S(6)|S(7)|S(8),	/* 0 */
    S(3)|S(4),				/* 1 */
    S(4)|S(5)|S(7)|S(8)|S(9),		/* 2 */
    S(3)|S(4)|S(5)|S(7)|S(9),		/* 3 */
    S(3)|S(4)|S(6)|S(9),		/* 4 */
    S(3)|S(5)|S(6)|S(7)|S(9),		/* 5 */
    S(3)|S(5)|S(6)|S(7)|S(8)|S(9),	/* 6 */
    S(3)|S(4)|S(5),			/* 7 */
    S(3)|S(4)|S(5)|S(6)|S(7)|S(8)|S(9),	/* 8 */
    S(3)|S(4)|S(5)|S(6)|S(7)|S(9),	/* 9 */
    S(9)				/* - */ 
};

#undef S

// number of segments (including decimal)
const float numi_brightness_scale[9] = { 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 };
const float numi_dac_scale = 1.0;
const float numi_dac_offset = 0.0;

//
// LCD Menu
//

#define MENU_TIMEOUT_PERIOD 5000

class menu_t {
public:
  struct leaf_item_t {
    std::string name;
    enum action_t { ACT_GET, ACT_UP, ACT_DOWN, ACT_ENTER } ;
    void (* func)(const std::string &name, action_t action, std::string &val);
  };

  struct top_item_t {
    std::string name;
    std::vector<leaf_item_t> leaf_items;
  };

  menu_t(LiquidCrystal &lcd, const std::vector<top_item_t> &top_items);
  void button(byte bt_id);

  void tick();
  bool lcd_enabled() { return st != ST_IDLE; }
  
  static const char *st_name[3];

private:
  enum { BT_MENU, BT_UP, BT_DOWN, BT_ENTER };
  enum state_t {
    ST_IDLE, ST_TOP, ST_LEAF
  };

  const std::vector<top_item_t> &top_items;
  LiquidCrystal &lcd;

  int top_item_idx;
  int leaf_item_idx;
  state_t st;
  
  elapsedMillis timer;

  void reset_timer() {
    timer = 0;
  };
  bool idle() {
    return st == ST_IDLE;
  }
};

const char *menu_t::st_name[3] = { "idle", "top", "leaf" };

menu_t::menu_t(LiquidCrystal &lcd, const std::vector<top_item_t> &top_items):
  top_items(top_items),
  lcd(lcd),
  top_item_idx(0),
  leaf_item_idx(0),
  st(ST_IDLE)
{}

void menu_t::button(byte bt_id)
{
  state_t next_st = st;

  std::string val;
  
  switch (st) {
  case ST_IDLE:
    // any button
    next_st = ST_TOP;
    top_item_idx = 0;
    break;

  case ST_TOP:
    switch (bt_id) {
    case BT_MENU:
      top_item_idx = (top_item_idx + 1) % top_items.size();
      break;

    case BT_ENTER:
      leaf_item_idx = 0;
      next_st = ST_LEAF;
      (*top_items[top_item_idx].leaf_items[leaf_item_idx].func)(top_items[top_item_idx].leaf_items[leaf_item_idx].name, leaf_item_t::ACT_GET, val);
      break;
    }
    break;

  case ST_LEAF:
    switch (bt_id) {
    case BT_MENU:
      leaf_item_idx = (leaf_item_idx + 1) % top_items[top_item_idx].leaf_items.size();
      break;

    case BT_UP:
      (*top_items[top_item_idx].leaf_items[leaf_item_idx].func)(top_items[top_item_idx].leaf_items[leaf_item_idx].name, leaf_item_t::ACT_UP, val);
     break;
    
    case BT_DOWN:
      (*top_items[top_item_idx].leaf_items[leaf_item_idx].func)(top_items[top_item_idx].leaf_items[leaf_item_idx].name, leaf_item_t::ACT_DOWN, val);
     break;

    case BT_ENTER:
      (*top_items[top_item_idx].leaf_items[leaf_item_idx].func)(top_items[top_item_idx].leaf_items[leaf_item_idx].name, leaf_item_t::ACT_ENTER, val);
      next_st = ST_TOP;
      break;
    }
    break;
  }

  if (st != next_st) {
    Log.trace("menu st: %s -> %s\n", st_name[st], st_name[next_st]);
 
    st = next_st;
  }

  if (!idle()) {
    lcd.display();
    lcd.setCursor(0,0);
    lcd.print(top_items[top_item_idx].name.c_str());
    Log.trace("menu %s.\n", top_items[top_item_idx].name.c_str());
    for (int i = top_items[top_item_idx].name.size(); i < 16; i ++) { // clear rest of row 0
      lcd.setCursor(i, 0);
      lcd.write(' ');
    }
    if (st == ST_LEAF) {
      lcd.setCursor(0,1);
      lcd.print(top_items[top_item_idx].leaf_items[leaf_item_idx].name.c_str());
      Log.trace("menu   .%s = %s\n", top_items[top_item_idx].leaf_items[leaf_item_idx].name.c_str(), val.c_str());
      for (int i = top_items[top_item_idx].leaf_items[leaf_item_idx].name.size(); i < 12; i ++) { // clear rest of name
	lcd.setCursor(i, 1);
	lcd.write(' ');
      }
      
      lcd.setCursor(12,1);
      lcd.print(val.c_str());
      for (int i = top_items[top_item_idx].leaf_items[leaf_item_idx].name.size(); i < 4; i ++) { // clear rest of row 1
	lcd.setCursor(12+i, 1);
	lcd.write(' ');
      }
    }
  }
  else {
    lcd.noDisplay();
  }

  reset_timer();
}

void menu_t::tick()
{
  if (!idle() && timer > MENU_TIMEOUT_PERIOD) {
    st = ST_IDLE;
    lcd.noDisplay();
    Log.trace("menu off\n");
  }
}

//
// Menu callbacks
//

void menu_brightness_numi(const std::string &nm, menu_t::leaf_item_t::action_t action, std::string &val)
{
  byte b = g_nv_options.st.numi_brightness;
  if (action == menu_t::leaf_item_t::ACT_UP && b < 255) {
    b ++;
  }
  else if (action == menu_t::leaf_item_t::ACT_DOWN && b > 0) {
    b --;
  }
  g_nv_options.st.numi_brightness = b;
  write_nvm();
  val = std::to_string(b);
}

void menu_brightness_colon(const std::string &nm, menu_t::leaf_item_t::action_t action, std::string &val)
{
  const byte res = 4;  // increment/decrement by 4

  byte b = g_nv_options.st.colon_brightness;
  if (action == menu_t::leaf_item_t::ACT_UP && b < (255-res)) {
    b += res;
  }
  else if (action == menu_t::leaf_item_t::ACT_DOWN && b > res) {
    b -= res;
  }
  g_nv_options.st.colon_brightness = b;
  write_nvm();
  val = std::to_string(b);
}

void menu_brightness_lcd(const std::string &nm, menu_t::leaf_item_t::action_t action, std::string &val)
{
  const byte res = 8; // increment/decremenet by 8
  byte b = g_nv_options.st.lcd_brightness;
  if (action == menu_t::leaf_item_t::ACT_UP && b < (255-res)) {
    b += res;
  }
  else if (action == menu_t::leaf_item_t::ACT_DOWN && b > res) {
    b -= res;
  }
  g_nv_options.st.lcd_brightness = b;
  write_nvm();
  val = std::to_string(b);
}

tmElements_t get_localtm()
{
  tmElements_t tm;
  breakTime(time_state.local, tm);
}

void set_localtm(const tmElements_t &tm)
{
  time_t new_local = makeTime(tm);
  time_state.set_utc(myTZ.toUTC(new_local));
  ds3231_set(time_state.utc);
  if (display_state == DISP_STATE_UNSET) {
    display_state = DISP_STATE_TIME;
  }
}

void menu_time_hour(const std::string &nm, menu_t::leaf_item_t::action_t action, std::string &val)
{
  tmElements_t tm(get_localtm());
  if (action == menu_t::leaf_item_t::ACT_UP && tm.Hour < 23) {
    tm.Hour ++;
  }
  else if (action == menu_t::leaf_item_t::ACT_DOWN && tm.Hour > 0) {
    tm.Hour --;
  }
  set_localtm(tm);
  val = std::to_string(tm.Hour);
}

void menu_time_min(const std::string &nm, menu_t::leaf_item_t::action_t action, std::string &val)
{
  tmElements_t tm(get_localtm());
  if (action == menu_t::leaf_item_t::ACT_UP && tm.Minute < 59) {
    tm.Minute ++;
  }
  else if (action == menu_t::leaf_item_t::ACT_DOWN && tm.Minute > 0) {
    tm.Minute --;
  }
  set_localtm(tm);
  val = std::to_string(tm.Minute);
}

void menu_time_sec(const std::string &nm, menu_t::leaf_item_t::action_t action, std::string &val)
{
  tmElements_t tm(get_localtm());
  tm.Second = 0;
  set_localtm(tm);
  val = std::to_string(tm.Second);
}

void menu_date_year(const std::string &nm, menu_t::leaf_item_t::action_t action, std::string &val)
{
  tmElements_t tm(get_localtm());
  if (action == menu_t::leaf_item_t::ACT_UP && tm.Year < 9999) {
    tm.Year ++;
  }
  else if (action == menu_t::leaf_item_t::ACT_DOWN && tm.Year > 1970) {
    tm.Year --;
  }
  set_localtm(tm);
  val = std::to_string(tm.Year);
}

void menu_date_month(const std::string &nm, menu_t::leaf_item_t::action_t action, std::string &val)
{
  tmElements_t tm(get_localtm());
  if (action == menu_t::leaf_item_t::ACT_UP && tm.Month < 11) {
    tm.Month ++;
  }
  else if (action == menu_t::leaf_item_t::ACT_DOWN && tm.Month > 0) {
    tm.Month --;
  }
  set_localtm(tm);
  val = std::to_string(tm.Month);
}

void menu_date_day(const std::string &nm, menu_t::leaf_item_t::action_t action, std::string &val)
{
  tmElements_t tm(get_localtm());
  if (action == menu_t::leaf_item_t::ACT_UP && tm.Day < 31) {
    tm.Day ++;
  }
  else if (action == menu_t::leaf_item_t::ACT_DOWN && tm.Day > 0) {
    tm.Day --;
  }
  set_localtm(tm);
  val = std::to_string(tm.Day);
}


std::vector<menu_t::top_item_t> lcd_menu_items = {
  { "Brightness",
    {
      { "LCD", menu_brightness_lcd },
      { "Numbers", menu_brightness_numi },
      { "Colon", menu_brightness_colon },
    }
  },
  { "Set time",
    {
      { "Hour", menu_time_hour },
      { "Minute", menu_time_min },
      { "Second", menu_time_sec },
    }
  },
  { "Set date",
    {
      { "Year", menu_date_year },
      { "Month", menu_date_month },
      { "Day", menu_date_day },
    }
  },
};

menu_t lcd_menu{lcd, lcd_menu_items};

//
// Initialization
//
void setup() {
   Serial.begin(9600);
   Log.begin(LOG_LEVEL_VERBOSE, &Serial);
   Log.trace("Starting\n");

   pinMode(LED_BUILTIN, OUTPUT);
  
   digitalWrite(PIN_DAC_LOAD, 1);
   digitalWrite(PIN_DAC_CLK, 0);
   pinMode(PIN_DAC_LOAD, OUTPUT);
   pinMode(PIN_DAC_CLK, OUTPUT);
   pinMode(PIN_DAC_DATA, OUTPUT);

   digitalWrite(PIN_TPIC_SCLK, 1);
   digitalWrite(PIN_TPIC_RCLK, 1);
   digitalWrite(PIN_TPIC_SDI, 1);
   digitalWrite(PIN_TPIC_OE_, 0);
   pinMode(PIN_TPIC_SCLK, OUTPUT);
   pinMode(PIN_TPIC_RCLK, OUTPUT);
   pinMode(PIN_TPIC_SDI, OUTPUT);
   pinMode(PIN_TPIC_OE_, OUTPUT);
 
   digitalWrite(PIN_BULB_PWM, 0);
   pinMode(PIN_BULB_PWM, OUTPUT);

   // set up the LCD's number of columns and rows:
   digitalWrite(PIN_LCD_BL_PWM, 0);
   pinMode(PIN_LCD_BL_PWM, OUTPUT);
   lcd.begin(16, 2);

   pinMode(PIN_RTC_SQW, INPUT_PULLUP);
   if (ds3231_setup()) {
     Log.trace("ds3231_setup succeeded\n");
     time_state.set_utc(ds3231_get_unixtime());
     Log.trace("Local time: ");
     printDateTime();
     display_state = DISP_STATE_TIME;
   }
   else {
     Serial.println("ds3231_setup failed");
     // time remains invalid until set by user
   }

   if (ds3231_getUserBytes(g_nv_options.get_packed_bytes(), g_nv_options.get_packed_size())) {
     if (g_nv_options.validate_checksum()) {
       Log.trace("Get valid user state; numi b %d, colon b %d, lcd b %d\n",
		 g_nv_options.st.numi_brightness,
		 g_nv_options.st.colon_brightness,
		 g_nv_options.st.lcd_brightness);
     }
     else {
       Log.trace("Bad user state; re-init\n");
       g_nv_options.init();
       write_nvm();
     }
   }
}

void update_numi_unset() {
  numi_digits[3] = 1;
  numi_digits[2] = 2;
  numi_digits[1] = 0;
  numi_digits[0] = 0;
  for (int i = 0; i < 4; i++) {
    numi_dec_points[i] = 1;
  }
   
}

void update_numi_time() {
  
  printDateTime();
  const byte hour12or24 = /*options.twentyfour_hour ? hour(time_state.local) :*/ hourFormat12(time_state.local);
  byte hour10 = hour12or24 / 10;
  hour10 = (hour10 > 0) ? hour10 : 15; // blank leading 0
  const byte pm = /*options.twentyfour_hour ? 0 :*/ isPM(time_state.local);
  numi_digits[3] = hour10;
  numi_digits[2] = hour12or24 % 10;
  if (0/*options.show_seconds*/) {
    numi_digits[1] = second(time_state.local) / 10;
    numi_digits[0] = second(time_state.local) % 10;
  } else {
    numi_digits[1] = minute(time_state.local) / 10;
    numi_digits[0] = minute(time_state.local) % 10;
  }
  for (int i = 0; i < 4; i++) {
    numi_dec_points[i] = 0;
  }
  numi_dec_points[1] = pm;
}


void pack_tpic(byte sdi[4]) {
  for (byte i = 0; i < 4; i ++) {
    sdi[i] = numi_digit_to_segments[numi_digits[i]];
    sdi[i] |= numi_dec_points[i] << 7;
  }
}

void update_tpic(const byte sdi[4])
{
  for (int i = 0; i < 32; i ++) {
    digitalWrite(PIN_TPIC_SDI, (sdi[i / 8] >> (7 - (i % 8))) & 0x1);
    digitalWrite(PIN_TPIC_SCLK, 1);
    digitalWrite(PIN_TPIC_SCLK, 0);
  }
  digitalWrite(PIN_TPIC_RCLK, 1);
  digitalWrite(PIN_TPIC_RCLK, 0);
  digitalWrite(PIN_TPIC_OE_, 0);
}

void tlc5620_dac_write(byte which, byte value)
{
  enum {ADDR=9, RNG=8};
  const uint16_t v16 = (which<<ADDR) | (0<<RNG) | value;
  for (byte b = 0; b < 11; b ++) {
      digitalWrite(PIN_DAC_CLK, 1);
      digitalWrite(PIN_DAC_DATA, (v16 >> (10 - b)) & 1);
      digitalWrite(PIN_DAC_CLK, 0);
    }
    digitalWrite(PIN_DAC_LOAD, 0);
    digitalWrite(PIN_DAC_LOAD, 1);
}

void printDateTime() {
  if (time_state.tcr == 0)
    return;
  
  time_t t = time_state.local;
  char m[4]; // temporary storage for month string (DateStrings.cpp uses shared buffer)
  strlcpy(m, monthShortStr(month(t)), 4);
  //Log.trace("%d:%d:%d %s %d %s %d %s\n", hour(t), minute(t), second(t),
  //	    dayShortStr(weekday(t)), day(t), m, year(t), time_state.tcr->abbrev);
}

void write_nvm() {
  ds3231_setUserBytes(g_nv_options.get_packed_bytes(), g_nv_options.get_packed_size());
}

void set_lcd_brightness(byte val)
{
  static byte last_b = 0;
  if (val != last_b) {
    Log.trace("set_lcd_brightness %d\n", val);
    last_b = val;
  }
  analogWrite(PIN_LCD_BL_PWM, val);
}

void set_colon_brightness(byte val)
{
  static byte last_b = 0;
  if (val != last_b) {
    Log.trace("set_colon_brightness %d\n", val);
    last_b = val;
  }
   analogWrite(PIN_BULB_PWM, val);
} 

//
// main loop
//

void loop()
{

  if (clkTimer > 200) {
    time_state.set_utc(ds3231_get_unixtime());
    clkTimer = 0;
  }

  if (buttonTimer > 200) {	// key repeat 200 ms
    buttonTimer = 0;
    digitalWrite(LED_BUILTIN,0);
    for (int i = 0; i < 2; i++) {
      if (touchRead(PIN_TOUCH[i]) > button_threshold[i]) {
	Log.trace("button %d\n", i);
	lcd_menu.button(i);
      }
    }
  }

  lcd_menu.tick();

  set_lcd_brightness(lcd_menu.lcd_enabled() ? g_nv_options.st.lcd_brightness : 0);
  set_colon_brightness(g_nv_options.st.colon_brightness);
  
  static byte test_val = 150;
  #if 0
  display_state = DISP_STATE_TEST;
  const byte DAC_MIN = 5;
  const byte DAC_MAX = 250;
  static byte test_dir = 0;

  if (test_dir==0) {
     if (test_val > DAC_MIN) { test_val -= 5; }
     else { test_dir = 1 - test_dir; }
  }
  else {
    if (test_val < DAC_MAX) { test_val += 5; }
    else { test_dir = 1 - test_dir;}
  }
  lcd.clear();
  lcd.print(test_val);

  set_lcd_brightness(test_val);
  set_colon_brightness(test_val);
  delay(200);
  #endif

  if (numiTimer > 200) {
    numiTimer = 0;
    byte numi_raw_brightness[4] = { g_nv_options.st.numi_brightness, g_nv_options.st.numi_brightness, g_nv_options.st.numi_brightness, g_nv_options.st.numi_brightness } ;
    byte tpic_data[4];
    
    switch (display_state) {
    case DISP_STATE_UNSET:
      update_numi_unset();
      pack_tpic(tpic_data);
      update_tpic(tpic_data);
      break;

    case DISP_STATE_TIME:
      if (time_state.new_time) {
	time_state.new_time = false;
	update_numi_time();
	pack_tpic(tpic_data);
	update_tpic(tpic_data);
      }
      break;
      
    case DISP_STATE_TEST:
      tpic_data[0] = 0xff;
      numi_raw_brightness[0] = test_val;
      update_tpic(tpic_data);
      break;
    }

    for (int i = 0; i < 4; i ++) {
      int val = (int)roundf((numi_brightness_scale[ __builtin_popcount(numi_digits[i]) + numi_dec_points[i] ]) * (float)numi_raw_brightness[i] + numi_dac_scale + numi_dac_offset);
      tlc5620_dac_write(0, constrain(val, 0, 255));
      //Log.trace("numi b[%d] = %d\n", i, val);
    }
  }
}
