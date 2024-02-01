//
// Teensy 3.2
//

// options:
// IV9: support IV9 Numitron on DB9 connector; else supports IV13
// DS3234: use DS3234 clock chip on SPI; else uses DS3231 clock chip on I2C
// OLED: use OLED display (SSD1306 128x32 I2C) instead of LCD
// BAD23: touch pad 23 bad, use 33 instead
#define IV9
#define DS3234
#define OLED
#define BAD23

#include <string>
#include <vector>
#include <elapsedMillis.h>
#include "LiquidCrystal.h"
#include <TimeLib.h>
#include <Timezone_Generic.h>
#include <ArduinoLog.h>
#include <RTClib.h>
#include <serial-readline.h>

#ifdef OLED
#include <AceWire.h>
//#include <AsyncDelay.h>
#define SSD1306_NO_SPLASH
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#endif

const byte PIN_DAC_CLK = 10;
const byte PIN_DAC_DATA = 11;
const byte PIN_DAC_LOAD = 12;

const byte PIN_TOUCH[] = {
  #ifdef BAD23
  33, 
  #else
  23
  #endif
  22, 16, 15};

#ifdef OLED
const byte PIN_OLED_SCL = 0;
const byte PIN_OLED_SDA = 1;
#else
const byte PIN_LCD_RS = 0;
const byte PIN_LCD_EN = 1;
const byte PIN_LCD_D4 = 2;
const byte PIN_LCD_D5 = 3;
const byte PIN_LCD_D6 = 4;
const byte PIN_LCD_D7 = 5;
const byte PIN_LCD_BL_PWM = 21;
#endif

const byte PIN_BULB_PWM = 20;

const byte PIN_TPIC_SCLK = 6;
const byte PIN_TPIC_RCLK = 7;
const byte PIN_TPIC_SDI = 8;
const byte PIN_TPIC_OE_ = 9;

// DS3231 RTC uses i2c: SDA = 18, SCL = 19
const byte PIN_RTC_SQW = 14;

// DS3234 RTC uses sofware SPI:
//    CLK 29
//    MISO 30
//    MOSI 31
//    CS 32

//
// LCD / OLED
//
#ifdef OLED
using ace_wire::SimpleWireInterface;
using WireInterface = SimpleWireInterface;
WireInterface sw(PIN_OLED_SDA, PIN_OLED_SCL, 5 /*uS delay*/);
// These buffers must be at least as large as the largest read or write you perform.
Adafruit_SSD1306 display(128, 32, &sw);

#else
LiquidCrystal lcd(PIN_LCD_RS, PIN_LCD_EN, PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7);
#endif


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
const int button_threshold[4] = {2500, 2500, 2500, 2500};

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
    st.colon_brightness = 48;
    st.lcd_brightness = 192;
    adjust_unpacked();
    update_checksum();
  }

  uint8_t calc_checksum() {
    uint8_t sum = 255;
    for (int i = 0; i < Size - 1; i++) {
      sum += unpackedBytes[i]; // modulo 8-bit sum
    }
    return sum;
  }
  
  void update_checksum() {
    st.checksum = calc_checksum();
  }

  bool validate_checksum() {
    unpack();
    adjust_unpacked();
    return calc_checksum() == st.checksum;
  }

  void adjust_unpacked() {
    // MSB replicate for those fields less than 8 bits
    st.lcd_brightness &= ~0x7;
    //st.lcd_brightness |= (st.lcd_brightness >> 5);
    st.colon_brightness &= ~0x3;
    //st.colon_brightness |= (st.colon_brightness >> 6);
  }

  uint8_t *get_packed_ptr() {
    return packedBytes;
  }
  
  const uint8_t *get_packed_bytes() {
    adjust_unpacked();
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
      uint8_t lcd_brightness;  // 0..255, 5 MSB bits
      uint8_t colon_brightness; // 0..255, 6 MSB bits
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
    tcr(0),
    twentyfour_hour(false),
    show_seconds(false)
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
  bool twentyfour_hour;
  bool show_seconds;
};
time_state_t time_state;

elapsedMillis clkTimer;

//
// Numitron IV-13
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
// Numitron IV-9
//
//   ---6--
//   |    |
//   5    3
//   ---4--
//   |    |
//   9    7
//   ---8--
//          *2
//

elapsedMillis numiTimer;

enum { DIG_0=0, DIG_1, DIG_2, DIG_3, DIG_4, DIG_5, DIG_6, DIG_7, DIG_8, DIG_9, DIG_DASH = 10, DIG_BLANK  };
byte numi_digits[4] = { DIG_DASH, DIG_DASH, DIG_DASH, DIG_DASH };
byte numi_dec_points[4] = {0,0,0,0};
byte test_tpic_data[4] = {0};

enum { DISP_STATE_UNSET, DISP_STATE_TIME, DISP_STATE_TEST, DISP_STATE_TESTRAW };
byte display_state = DISP_STATE_UNSET;

#define S(n) (1<<(n-2))

static const uint16_t numi_digit_to_segments[12] = {
#ifdef IV9
    S(3)|S(5)|S(6)|S(7)|S(8)|S(9),      /* 0 */
    S(3)|S(7),                          /* 1 */
    S(3)|S(4)|S(6)|S(8)|S(9),           /* 2 */
    S(3)|S(4)|S(6)|S(7)|S(8),           /* 3 */
    S(3)|S(4)|S(5)|S(7),                /* 4 */
    S(4)|S(5)|S(6)|S(7)|S(8),           /* 5 */
    S(4)|S(5)|S(6)|S(7)|S(8)|S(9),      /* 6 */
    S(3)|S(6)|S(7),                     /* 7 */
    S(3)|S(4)|S(5)|S(6)|S(7)|S(8)|S(9), /* 8 */
    S(3)|S(4)|S(5)|S(6)|S(7)|S(8),      /* 9 */
    S(4),                               /* - */ 
    0                                   /*   */ 
#else
    S(3)|S(4)|S(5)|S(6)|S(7)|S(8),      /* 0 */
    S(3)|S(4),                          /* 1 */
    S(4)|S(5)|S(7)|S(8)|S(9),           /* 2 */
    S(3)|S(4)|S(5)|S(7)|S(9),           /* 3 */
    S(3)|S(4)|S(6)|S(9),                /* 4 */
    S(3)|S(5)|S(6)|S(7)|S(9),           /* 5 */
    S(3)|S(5)|S(6)|S(7)|S(8)|S(9),      /* 6 */
    S(3)|S(4)|S(5),                     /* 7 */
    S(3)|S(4)|S(5)|S(6)|S(7)|S(8)|S(9), /* 8 */
    S(3)|S(4)|S(5)|S(6)|S(7)|S(9),      /* 9 */
    S(9),                               /* - */ 
    0                                   /*   */ 
#endif
      };

#undef S

// number of segments (including decimal)
const float numi_brightness_scale[9] = { 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0 };
const float numi_dac_scale = 1.0;
const float numi_dac_offset = 0.0;

//
// LCD Menu
//

#define MENU_TIMEOUT_PERIOD 60000

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

#ifdef OLED
  menu_t(Adafruit_SSD1306 &display, const std::vector<top_item_t> &top_items);
#else
  menu_t(LiquidCrystal &lcd, const std::vector<top_item_t> &top_items);
#endif  
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
#ifdef OLED
  Adafruit_SSD1306 &display;
#else
  LiquidCrystal &lcd;
#endif

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

menu_t::menu_t(
#ifdef OLED
  Adafruit_SSD1306 &display,
#else
   LiquidCrystal &lcd, 
#endif
  const std::vector<top_item_t> &top_items):
  top_items(top_items),
#ifdef OLED
  display(display),
#else
  lcd(lcd),
#endif
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
      (*top_items[top_item_idx].leaf_items[leaf_item_idx].func)(top_items[top_item_idx].leaf_items[leaf_item_idx].name, leaf_item_t::ACT_GET, val);
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

#ifdef OLED
  if (!idle()) {
    display.ssd1306_command(SSD1306_DISPLAYON);
    display.clearDisplay();
    display.setTextSize(2);      // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE); // Draw white text
    display.setCursor(0, 0);     // Start at top-left corner
  //display.cp437(true);         // Use full 256 char 'Code Page 437' font

    display.println(top_items[top_item_idx].name.c_str());
    Log.trace("menu %s.\n", top_items[top_item_idx].name.c_str());

    if (st == ST_LEAF) {
      display.setCursor(0,16);
      display.print(top_items[top_item_idx].leaf_items[leaf_item_idx].name.c_str());
      Log.trace("menu   .%s = %s\n", top_items[top_item_idx].leaf_items[leaf_item_idx].name.c_str(), val.c_str());
      
      
      display.setCursor(80,16);
      display.print(val.c_str());
      
    }
    
    display.display();
  }
  else {
    display.ssd1306_command(SSD1306_DISPLAYOFF);
  }
#else
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
      for (int i = val.size(); i < 4; i ++) { // clear rest of value
              lcd.setCursor(12+i, 1);
              lcd.write(' ');
      }
    }
    else { // clear 2nd row
      lcd.setCursor(0,1);
      for (int i = 0; i < 16; i ++) {
              lcd.write(' ');
      }
    }
  }
  else {
    lcd.noDisplay();
  }
#endif

  reset_timer();
}

void menu_t::tick()
{
  if (!idle() && timer > MENU_TIMEOUT_PERIOD) {
    st = ST_IDLE;
#ifdef OLED
    display.ssd1306_command(SSD1306_DISPLAYOFF);
#else
    lcd.noDisplay();
#endif
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
  return tm;
}

void set_localtm(const tmElements_t &tm)
{
  time_t new_local = makeTime(tm);
  time_state.set_utc(myTZ.toUTC(new_local));
  #ifdef DS3234
  ds3234_set(time_state.utc);
  #else
  ds3231_set(time_state.utc);
  #endif
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
  if (action == menu_t::leaf_item_t::ACT_UP && tm.Year < 199) {
    tm.Year ++;
  }
  else if (action == menu_t::leaf_item_t::ACT_DOWN && tm.Year > 0) {
    tm.Year --;
  }
  set_localtm(tm);
  val = std::to_string(tm.Year + 1970);  // year 0 = 1970
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
      { 
#ifdef OLED
        "Screen",
#else       
        "LCD",
#endif
        menu_brightness_lcd },
      { "Digits", menu_brightness_numi },
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

menu_t lcd_menu{
  #ifdef OLED
  display,
  #else
  lcd, 
  #endif
  lcd_menu_items};

//
// Serial port command line
//

void serial_received(char *s)
{
  String str(s);
  cmd_parse(str);
}

SerialLineReader<typeof(Serial)>  serial_reader(Serial, serial_received);

//
// Initialization
//
void setup() {
   Serial.begin(9600);
   Log.begin(LOG_LEVEL_VERBOSE, &Serial);
   Log.trace("Starting\n");

   //pinMode(LED_BUILTIN, OUTPUT);
  
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

#ifdef OLED
  sw.begin();
  display.begin();
  display.ssd1306_command(SSD1306_DISPLAYOFF);
#else
   // set up the LCD's number of columns and rows:
   digitalWrite(PIN_LCD_BL_PWM, 0);
   pinMode(PIN_LCD_BL_PWM, OUTPUT);
   lcd.begin(16, 2);
#endif

   pinMode(PIN_RTC_SQW, INPUT_PULLUP);
   #ifdef DS3234
   if (ds3234_setup()) {
     Log.trace("ds3234_setup succeeded\n");
     time_state.set_utc(ds3234_get_unixtime());
     Log.trace("Local time: ");
     printDateTime();
     display_state = DISP_STATE_TIME;
   }
   else {
     Serial.println("ds3234_setup failed");
     // time remains invalid until set by user
   }
   #else
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
   #endif

   if (
    #ifdef DS3234
    ds3234_getUserBytes(g_nv_options.get_packed_ptr(), g_nv_options.get_packed_size())
    #else
    ds3231_getUserBytes(g_nv_options.get_packed_ptr(), g_nv_options.get_packed_size())
    #endif
    ) {
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
  numi_digits[0] = DIG_DASH;
  numi_digits[1] = DIG_DASH;
  numi_digits[2] = DIG_DASH;
  numi_digits[3] = DIG_DASH;
  for (int i = 0; i < 4; i++) {
    numi_dec_points[i] = 1;
  }
   
}

void update_numi_time()
{

  static byte prev_min = 0;
  if (minute(time_state.local) != prev_min) {
    printDateTime();
    prev_min = minute(time_state.local);
  }
  const byte hour12or24 = time_state.twentyfour_hour ? hour(time_state.local) : hourFormat12(time_state.local);
  byte hour10 = hour12or24 / 10;
  hour10 = (hour10 > 0) ? hour10 : DIG_BLANK; // blank leading 0
  const byte pm = time_state.twentyfour_hour ? 0 : isPM(time_state.local);
  numi_digits[0] = hour10;
  numi_digits[1] = hour12or24 % 10;
  if (time_state.show_seconds) {
    numi_digits[2] = second(time_state.local) / 10;
    numi_digits[3] = second(time_state.local) % 10;
  } else {
    numi_digits[2] = minute(time_state.local) / 10;
    numi_digits[3] = minute(time_state.local) % 10;
  }
  for (int i = 0; i < 4; i++) {
    numi_dec_points[i] = 0;
  }
  numi_dec_points[1] = pm;
}

void pack_tpic(byte sdi[4])
{
  for (byte i = 0; i < 4; i ++) {
    sdi[i] = numi_digit_to_segments[numi_digits[i]];
    sdi[i] |= numi_dec_points[i] & 1;
  }
}

void update_tpic(const byte sdi[4])
{
  static byte prev_sdi[4] = {0};
  for (int i = 0; i < 32; i ++) {
    digitalWrite(PIN_TPIC_SDI, (sdi[i / 8] >> (7 - (i % 8))) & 0x1);
    digitalWrite(PIN_TPIC_SCLK, 1);
    digitalWrite(PIN_TPIC_SCLK, 0);
  }
  digitalWrite(PIN_TPIC_RCLK, 1);
  digitalWrite(PIN_TPIC_RCLK, 0);
  digitalWrite(PIN_TPIC_OE_, 0);
  if (memcmp(sdi, prev_sdi, sizeof(prev_sdi))) {
    Log.trace("update_tpic %x %x %x %x\n", sdi[0], sdi[1], sdi[2], sdi[3]);
    memcpy(prev_sdi, sdi, sizeof(prev_sdi));
  }
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
  Log.trace("%d:%d:%d %s %d %s %d %s\n", hour(t), minute(t), second(t),
            dayShortStr(weekday(t)), day(t), m, year(t), time_state.tcr->abbrev);
}

void write_nvm() {
  #ifdef DS3234
  ds3234_setUserBytes(g_nv_options.get_packed_bytes(), g_nv_options.get_packed_size());
  #else
  ds3231_setUserBytes(g_nv_options.get_packed_bytes(), g_nv_options.get_packed_size());
  #endif
}

void set_lcd_brightness(byte val)
{
  static byte last_b = 0;
  if (val != last_b) {
    Log.trace("set_lcd_brightness %d\n", val);
    last_b = val;
  }
#ifdef OLED
#else
  analogWrite(PIN_LCD_BL_PWM, val);
#endif
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

void cmd_parse(String &bt_cmd) {
  int v = 0, vx = 0;
  int i;
  if ((i = bt_cmd.indexOf(':')) >= 0) {
    String value_s(bt_cmd.substring(i + 1));
    v = atoi(value_s.c_str());
    vx = (int)strtol(value_s.c_str(), 0, 16);
  }

  time_t new_utc = time_state.utc;
  if (bt_cmd.startsWith("nb:")) {
    if (g_nv_options.st.numi_brightness != v) {
      g_nv_options.st.numi_brightness = v;
      write_nvm();
    }  
  }
  else if (bt_cmd.startsWith("cb:")) {
    if (g_nv_options.st.colon_brightness != v) {
      g_nv_options.st.colon_brightness = v;
      write_nvm();
    }
  }
  else if (bt_cmd.startsWith("lb:")) {
    if (g_nv_options.st.lcd_brightness != v) {
      g_nv_options.st.lcd_brightness = v;
      write_nvm();
    }
  }
  else if (bt_cmd.startsWith("t:")) {
    display_state = (v == 2) ? DISP_STATE_TESTRAW : (v == 1) ? DISP_STATE_TEST : DISP_STATE_TIME;
    Log.trace("test state %s\n", (v == 2) ? "TESTRAW" : (v == 1) ? "TEST" : "-");
  }
  else if (bt_cmd.startsWith("cs:")) {
    if (
      #ifdef DS3234
      ds3234_getUserBytes(g_nv_options.get_packed_ptr(), g_nv_options.get_packed_size())
      #else
      ds3231_getUserBytes(g_nv_options.get_packed_ptr(), g_nv_options.get_packed_size())
      #endif
      ) {
     if (g_nv_options.validate_checksum()) {
       Log.trace("Get valid user state; numi b %d, colon b %d, lcd b %d\n",
                 g_nv_options.st.numi_brightness,
                 g_nv_options.st.colon_brightness,
                 g_nv_options.st.lcd_brightness);
     }
     else {
       Log.warning("Bad user state; re-init\n");
     }
   }
  }
  else if (bt_cmd.startsWith("s0:")) {
    test_tpic_data[0] = vx;
  }
  else if (bt_cmd.startsWith("s1:")) {
    test_tpic_data[1] = vx;
  }
  else if (bt_cmd.startsWith("s2:")) {
    test_tpic_data[2] = vx;
  }
  else if (bt_cmd.startsWith("s3:")) {
    test_tpic_data[3] = vx;
  }
  else if (bt_cmd.startsWith("d0:")) {
    numi_digits[0] = v % 11;
  }
  else if (bt_cmd.startsWith("d1:")) {
    numi_digits[1] = v % 11;
  }
  else if (bt_cmd.startsWith("d2:")) {
    numi_digits[2] = v % 11;
  }
  else if (bt_cmd.startsWith("d3:")) {
    numi_digits[3] = v % 11;
  }
  else if (bt_cmd.startsWith("dp0:")) {
    numi_dec_points[0] = v % 2;
  }
  else if (bt_cmd.startsWith("dp1:")) {
    numi_dec_points[1] = v % 2;
  }
  else if (bt_cmd.startsWith("dp2:")) {
    numi_dec_points[2] = v % 2;
  }
  else if (bt_cmd.startsWith("dp3:")) {
    numi_dec_points[3] = v % 2;
  }
  else if (bt_cmd.startsWith("mb0:")) {
    lcd_menu.button(0);
  }
  else if (bt_cmd.startsWith("mb1:")) {
    lcd_menu.button(1);
  }
  else if (bt_cmd.startsWith("mb2:")) {
    lcd_menu.button(2);
  }
  else if (bt_cmd.startsWith("mb3:")) {
    lcd_menu.button(3);
  }
  else if (bt_cmd.startsWith("h:") && (v != 0)) {
    new_utc += v * 3600;
  }
  else if (bt_cmd.startsWith("m:") && (v != 0)) {
    new_utc += v * 60;
  }
  else if (bt_cmd.startsWith("s:") && (v != 0)) {
    new_utc += v;
  }
  else if (bt_cmd.startsWith("set-time:")) { // local "ISO 8601" format e.g. 2020-06-25T15:29:37
      time_t local_time = datetime_parse(bt_cmd.substring(bt_cmd.indexOf(':') + 1));
      new_utc = myTZ.toUTC(local_time);
  }
  else if (bt_cmd.startsWith("gt:")) {
    printDateTime();
  }
  else if (bt_cmd.startsWith("ss:")) {
    time_state.show_seconds = v;
  }
  else if (bt_cmd.startsWith("24:")) {
    time_state.twentyfour_hour = v;
  }
  else {
    Log.warning("Bad cmd\n");
  }
   
  if (time_state.utc != new_utc) {
    time_state.set_utc(new_utc);
    #ifdef DS3234
    ds3234_set(new_utc);
    #else
    ds3231_set(new_utc);
    #endif
    Log.trace("Set time:\n");
    printDateTime();
    display_state = DISP_STATE_TIME;
  }
}

//
// main loop
//

void loop()
{
  serial_reader.poll();
  
  if (clkTimer > 333) {
    time_state.set_utc(
    #ifdef DS3234
    ds3234_get_unixtime()
    #else
    ds3231_get_unixtime()
    #endif
    );
    clkTimer = 0;
  }

  if (buttonTimer > 200) {      // key repeat 200 ms
    buttonTimer = 0;
    //digitalWrite(LED_BUILTIN,0);
    int button[4];
    for (int i = 0; i < 4; i++) {
      button[i] = touchRead(PIN_TOUCH[i]);
      if (button[i] > button_threshold[i]) {
        Log.trace("button %d\n", i);
        lcd_menu.button(i);
      }
    }
    Log.trace("%d, %d, %d, %d\n", button[0], button[1], button[2], button[3]);
  }

  lcd_menu.tick();

  set_lcd_brightness(lcd_menu.lcd_enabled() ? g_nv_options.st.lcd_brightness : 0);
  set_colon_brightness(g_nv_options.st.colon_brightness);
  
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
      // numi_digits set by cmd
      pack_tpic(tpic_data);
      update_tpic(tpic_data);
      break;

    case DISP_STATE_TESTRAW:
      // tpic data set by cmd
      update_tpic(test_tpic_data);
      break;
    }

    static byte last_val[4] = {0};
    for (int i = 0; i < 4; i ++) {
      int val;
      switch (display_state) {
      case DISP_STATE_TESTRAW:
        val = numi_raw_brightness[i];
        break;
      default:
        val = (int)roundf((numi_brightness_scale[ __builtin_popcount(numi_digits[i]) + numi_dec_points[i] ]) * (float)numi_raw_brightness[i] * numi_dac_scale + numi_dac_offset);
        break;
      }
      byte val8 = constrain(val, 0, 255);
      tlc5620_dac_write(i, val8);
      if (val8 != last_val[i]) {
        Log.trace("numi final bright[%d] = %d\n", i, val8);
        last_val[i] = val8;
      }
    }
  }
}
