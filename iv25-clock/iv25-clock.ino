//
//  for SparkFun Pro Micro 5V 
//

#include <TimeLib.h>
#include <Timezone_Generic.h>
#include <elapsedMillis.h>
#include "ir.h"

static const int PIN_CLK = 6;
static const int PIN_SDI = 7;
static const int PIN_STROBE = 8;
static const int PIN_OE_ = 9;
static const int PIN_LED = 17;

extern bool ds3231_setup();
extern time_t ds3231_get_unixtime(void); 

struct nonvolatile_state_t {
    nonvolatile_state_t() {
        init();
    }

    void init()
    {
        st.brightness = 255;
        for (int i=0; i < sizeof(st.reserved); i++) {
            st.reserved[i] = 0;
        }
        update_checksum();
    }
    
    void update_checksum() {
        st.checksum = 255;
        for (int i=0; i < sizeof(bytes) - 1; i++) {
            st.checksum += bytes[i];
        }
    }

    bool validate_checksum() {
        uint8_t sum = 255;
        for (int i=0; i < sizeof(bytes) - 1; i++) {
            sum += bytes[i];
        }
        return sum == st.checksum;
    }

    uint8_t *get_bytes() {
        update_checksum();
        return bytes;
    }
    int get_size() {
        return Size;
    }

    static const int Size = 7;
    union {
        uint8_t bytes[Size];
        struct {
            uint8_t brightness;
            uint8_t reserved[Size-2];
            uint8_t checksum;
        } st;
    };
};

template <int C,class T> class my_vector {
public:
    my_vector():
        m_size(0)
    {}
    int size() { return m_size; }
    void push_back(T e) {
        if (m_size < capacity)
            data[m_size ++] = e;
    }
    void clear() {
        m_size = 0;
    }
    T &operator [] (int i) {
        return data[i];
    }

private:
    const int capacity = C;
    int m_size;
    T data[C];
};

//
// global state
//
nonvolatile_state_t g_state;

//
// IR
//
my_vector<12, uint8_t> ir_nums;

//
// TimeZone
//
TimeChangeRule myPDT = {"PDT", Second, Sun, Mar,
                        2,     -60 * 7}; // Daylight time = UTC - 7 hours
TimeChangeRule myPST = {"PST", First, Sun,
                        Nov,   2,     -60 * 8}; // Standard time = UTC - 8 hours
Timezone myTZ(myPDT, myPST);
TimeChangeRule *tcr; // pointer to the time change rule, use to get TZ abbrev

void setup() {
  Serial.begin(9600);
  delay(1500);
  Serial.println("Start");
 pinMode(PIN_LED, OUTPUT);

 pinMode(PIN_CLK, OUTPUT);
 pinMode(PIN_SDI, OUTPUT);
 pinMode(PIN_STROBE, OUTPUT);
 pinMode(PIN_OE_, OUTPUT);
 digitalWrite(PIN_CLK, 0);
 digitalWrite(PIN_SDI, 0);
 digitalWrite(PIN_STROBE, 0);
 digitalWrite(PIN_OE_, 1);

  if (ds3231_setup()) {
     Serial.println("ds3231_setup succeeded");
     time_t utc = ds3231_get_unixtime();
     time_t local = myTZ.toLocal(utc, &tcr);
     Serial.println("Local time:");
     printDateTime(local, tcr->abbrev);
  }
  else {
    Serial.println("ds3231_setup failed; setting");
   // local "ISO 8601" format e.g. 2020-06-25T15:29:37
      time_t local_time = datetime_parse("2021-09-04T19:38:00");
      time_t new_utc = myTZ.toUTC(local_time);
      Serial.print("Set time: ");
      printDateTime(new_utc, tcr->abbrev);
  
    ds3231_set(new_utc);
  }
  if (ds3231_getUserBytes(g_state.get_bytes(), g_state.get_size())) {
      if (g_state.validate_checksum()) {
          Serial.print("Get valid user state; b "); Serial.println(g_state.st.brightness);
      }
      else {
          Serial.println("Bad user state; re-init");
          g_state.init();
          ds3231_setUserBytes(g_state.get_bytes(), g_state.get_size());
      }
  }
  ir_setup();
  
  bt_setup();
}

const uint8_t map_seg_to_bit[7] = {
  5, 6, 1, 0, 4, 3, 2
};

const uint8_t NUM_DIGITS = 12;
const uint8_t TUBE_PER_DIG = 2;
uint8_t sdi_d[NUM_DIGITS * TUBE_PER_DIG] = {0};
const int TUBE(uint8_t digit, uint8_t lo_hi) {
    return digit * TUBE_PER_DIG + lo_hi;
}
bool first_vfd_update = false;

int test_mode = 0;

void do_ir()
{
  ir_cmd_t ir_cmd;
  bool ir_rep;
  if (ir_check(ir_cmd, ir_rep)) {
      switch (ir_cmd) {
          //
          // accumulate entered number
          //
          case IR_0:
              if (!ir_rep) ir_nums.push_back(0);
              break;
          case IR_1:
              if (!ir_rep) ir_nums.push_back(1);
              break;
          case IR_2:
              if (!ir_rep) ir_nums.push_back(2);
              break;
          case IR_3:
              if (!ir_rep) ir_nums.push_back(3);
              break;
          case IR_4:
              if (!ir_rep) ir_nums.push_back(4);
              break;
          case IR_5:
              if (!ir_rep) ir_nums.push_back(5);
              break;
          case IR_6:
              if (!ir_rep) ir_nums.push_back(6);
              break;
          case IR_7:
              if (!ir_rep) ir_nums.push_back(7);
              break;
          case IR_8:
              if (!ir_rep) ir_nums.push_back(8);
              break;
          case IR_9:
              if (!ir_rep) ir_nums.push_back(9);
              break;

          case IR_FUNCSTOP:
              ir_nums.clear();
              break;

          case IR_STREPT:
              if (!ir_rep && ir_nums.size() > 0) {
                  int val = 0;
                  for (int i = 0; i < ir_nums.size(); i ++) {
                      val = val*10 + i;
                  }
                  set_brightness(val);
                  ir_nums.clear();
              }
              break;

          case IR_PLAY:
              if (!ir_rep && ir_nums.size() >= 2) {
                  time_t utc = ds3231_get_unixtime();
                  time_t local = myTZ.toLocal(utc, &tcr);
                  tmElements_t tm;
                  breakTime(local, tm);
                  int i = 0;
                  if (ir_nums.size() == 12) {
                      tm.Year = 2000 - 1970 + (ir_nums[i] * 10 + ir_nums[i+1]);
                      i += 2;
                  }
                  if (ir_nums.size() >= 10) {
                      tm.Month = ir_nums[i] * 10 + ir_nums[i+1];
                      i += 2;
                  }
                  if (ir_nums.size() >= 8) {
                      tm.Day = ir_nums[i] * 10 + ir_nums[i+1];
                      i += 2;
                  }
                  if (ir_nums.size() >= 6) {
                      tm.Hour = ir_nums[i] * 10 + ir_nums[i+1];
                      i += 2;
                  }
                  if (ir_nums.size() >= 4) {
                      tm.Minute = ir_nums[i] * 10 + ir_nums[i+1];
                      i += 2;
                  }
                  if (ir_nums.size() >= 2) {
                      tm.Second = ir_nums[i] * 10 + ir_nums[i+1];
                      i += 2;
                  }
                  time_t new_local = makeTime(tm);
                  time_t new_utc = myTZ.toUTC(new_local);
                  Serial.print("Set time: ");
                  printDateTime(new_utc, tcr->abbrev);
                  ds3231_set(new_utc);
                  ir_nums.clear();
              }
              break;

          case IR_REW:
              if (!ir_rep) {
                  time_t utc = ds3231_get_unixtime();
                  time_t local = myTZ.toLocal(utc, &tcr);
                  Serial.print("Get time: ");
                  printDateTime(local, tcr->abbrev);
              }
              break;
              
          case IR_VOLUP:
              set_brightness(min(255, g_state.st.brightness + 1));
              break;
          case IR_VOLDOWN:
              set_brightness(max(0, g_state.st.brightness - 1));
              break;

          case IR_EQ:
              if (!ir_rep && ir_nums.size() > 1) {
                  int val = 0;
                  for (int i = 0; i < ir_nums.size(); i ++) {
                      val = val*10 + i;
                  }
                  test_mode = val;
                  ir_nums.clear();
                  Serial.print("Set test mode "); Serial.println(test_mode);
              }
              break;
      }
  }
}

void set_brightness(uint8_t b) {
    b = max(0, min(b, 255));
    analogWrite(PIN_OE_, 255 - b);
    Serial.print("Set brightness: "); Serial.println(b);
    if (g_state.st.brightness != b) {
        g_state.st.brightness = b;
        ds3231_setUserBytes(g_state.get_bytes(), g_state.get_size());
    }
}

void update_vfd()
{
    for (int i = 0; i < NUM_DIGITS * TUBE_PER_DIG * 8; i ++) {
        digitalWrite(PIN_SDI, (sdi_d[NUM_DIGITS * TUBE_PER_DIG - 1 - i/8] >> (7 - (i%8))) & 0x1);
        digitalWrite(PIN_CLK, 1);
        digitalWrite(PIN_CLK, 0);
    }
    digitalWrite(PIN_STROBE, 1);
    digitalWrite(PIN_STROBE, 0);
    if (!first_vfd_update) {
        set_brightness(g_state.st.brightness);
        first_vfd_update = true;
    }
}

void time_to_vfd()
{
    const time_t utc_time = ds3231_get_unixtime();
    const time_t local_time = myTZ.toLocal(utc_time, &tcr);

    memset(sdi_d, 0, sizeof(sdi_d));

    int hr_tube0 = TUBE(hour(local_time), 0);
    int hr_tube1 = TUBE(hour(local_time), 1);
    int min_tube0 = TUBE(minute(local_time) / 5, 0);
    int min_tube1 = TUBE(minute(local_time) / 5, 1);

    // hour: set all 7 of lower tube of hour segment
    for (int i = 0; i < 7; i ++) {
        sdi_d[hr_tube0] |= 1<<map_seg_to_bit[i];
    }
    // minute:
    //    for minutes/5: set all 7 of lower tube, and
    //                   set bottom 2 of upper tube 
    //    for minutes%5: set 0-5 of upper 5 of upper tube:
    //               %5 == 0: set all 5
    //               %5 == 1..4: set 1..4 
    for (int i = 0; i < 7; i ++) {
        sdi_d[min_tube0] |= 1<<map_seg_to_bit[i];
    }
    for (int i = 0; i < 2; i ++) {
        sdi_d[min_tube1] |= 1<<map_seg_to_bit[i];
    }
    int minutes_extra = minute(local_time) % 5;
    if (minutes_extra == 0) {
        minutes_extra = 5;
    }
    for (int i = 2; i < 2 + minutes_extra; i ++) {
        sdi_d[min_tube1] |= 1<<map_seg_to_bit[i];
    }

    // second: flash remaining 10 segments' lowest
    for (int i = 0; i < 12; i ++) {
        int sec_tube = TUBE(second(local_time), 0);
        if (sec_tube != hr_tube0 && sec_tube != min_tube0) {
            sdi_d[sec_tube] |= (second(local_time) & 1) << map_seg_to_bit[0];
        }
    }
    digitalWrite(PIN_LED, (second(local_time) & 1));
    update_vfd();
}

void do_test_vfd() {
    static int16_t cnt = 0;
  uint8_t digit = cnt/14;
  uint8_t lo_hi = (cnt/7) % 2;
  uint8_t seg = cnt % 7;
  memset(sdi_d, 0, sizeof(sdi_d));
  
   //sdi_d[TUBE(digit,lo_hi)] |= 1 << map_seg_to_bit[seg];
  for (uint8_t tube = 0 ; tube < 6; tube ++) {
   sdi_d[tube] = (cnt&1) ? (1<<map_seg_to_bit[tube]) : ~(1<<map_seg_to_bit[tube]);
  }
  update_vfd();
  
   cnt=(cnt+1)%28;
   digitalWrite(PIN_LED, cnt&1);
}

elapsedMillis vfd_timer;

void loop() {
  do_ir();

  if (vfd_timer >= 250) {
      if (test_mode == 1) {
          do_test_vfd();
      }
      else {
          time_to_vfd();
      }
      vfd_timer = 0;
  }

  String bt_cmd;
  if (bt_loop(bt_cmd)) {
      if (bt_cmd.startsWith("b ")) {
          int pwm = atoi(bt_cmd.substring(bt_cmd.indexOf(' ')).c_str());
          Serial.print("bt pwm:"); Serial.println(pwm);
          set_brightness(pwm);
      }
      else if (bt_cmd.startsWith("st ")) { // local "ISO 8601" format e.g. 2020-06-25T15:29:37
          time_t local_time = datetime_parse(bt_cmd.substring(bt_cmd.indexOf(' ') + 1));
          time_t new_utc = myTZ.toUTC(local_time);

          Serial.print("bt set time: ");
          printDateTime(local_time, tcr->abbrev);
          ds3231_set(new_utc);
      }

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
