//
//  for SparkFun Pro Micro 5V 
//

#include <TimeLib.h>
#include <Timezone_Generic.h>
#include <elapsedMillis.h>
#include "ir.h"

// RX/TX Serial for HM-10: pin 0/1
// I2C (RTC) pin 2/3
// IR Pin 4
static const int PIN_CLK = 6;
static const int PIN_SDI = 7;
static const int PIN_STROBE = 8;
static const int PIN_OE_ = 9;
static const int PIN_LED = 17;  // builtin

struct nonvolatile_state_t {
    nonvolatile_state_t() {
        init();
    }

    void init()
    {
        memset(unpackedBytes, 0, Size);
        memset(packedBytes, 0, PackedSize);
        st.brightness = 255;
        st.rotation = 0;
        update_checksum();
    }
    
    void update_checksum() {
        st.checksum = 255;
        for (int i=0; i < Size - 1; i++) {
            st.checksum += unpackedBytes[i]; // modulo 8-bit sum
        }
    }

    bool validate_checksum() {
        unpack();
        uint8_t sum = 255;
        for (int i=0; i < Size - 1; i++) {
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
    _( 1,      4,    1,       0,   2) /* rotation[1:0] - Alarm 1 10 minutes   */ \
    _( 2,      0,    1,       2,   2) /* rotation[2:2] - Alarm 1 Hour         */ \
    _( 3,      0,    2,       0,   3) /* checksum[2:0] - Alarm 1 Date         */ \
    _( 4,      0,    5,       3,   3) /* checksum[5:3] - Alarm 2 minutes      */ \
    _( 4,      4,    7,       6,   2) /* checksum[7:6] - Alarm 2 10 minutes   */
    
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
            uint8_t brightness; // 0..255
            uint8_t rotation; // 0..11
            uint8_t reserved[Size - 2 - 1];
            uint8_t checksum;
        } st;
    };

    // packed size is 7 bytes, which is the size of DS3231 Alarm1 + Alarm2 registers.
    // However, not all the fields in them are used.
    const static int PackedSize = 7;
    uint8_t packedBytes[PackedSize];
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
bool g_time_valid = false;
time_t g_local_time = 0;
bool g_first_vfd_update = false;
int g_test_mode = 1;

nonvolatile_state_t g_nv_state;

//
// IR
//
my_vector<12, uint8_t> ir_nums; // sized for YYMMDDHHMMSS

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
  delay(2000);
  Serial.println("Start");
  
  pinMode(PIN_LED, OUTPUT);

  pinMode(PIN_CLK, OUTPUT);
  pinMode(PIN_SDI, OUTPUT);
  pinMode(PIN_STROBE, OUTPUT);
  pinMode(PIN_OE_, OUTPUT);
  digitalWrite(PIN_CLK, 0);
  digitalWrite(PIN_SDI, 0);
  digitalWrite(PIN_STROBE, 0);
  digitalWrite(PIN_OE_, 1);     // enable VFD only after values initialized

  if (ds3231_setup()) {
     Serial.println("ds3231_setup succeeded");
     time_t utc = ds3231_get_unixtime();
     time_t local = myTZ.toLocal(utc, &tcr);
     Serial.println("Local time:");
     printDateTime(local, tcr->abbrev);
     g_time_valid = true;
  }
  else {
    Serial.println("ds3231_setup failed");
    // time remains invalid until set by user
  }

  if (ds3231_getUserBytes(g_nv_state.get_packed_bytes(), g_nv_state.get_packed_size())) {
      if (g_nv_state.validate_checksum()) {
          Serial.print("Get valid user state; b "); Serial.println(g_nv_state.st.brightness);
          Serial.print("   rot "); Serial.println(g_nv_state.st.rotation);
      }
      else {
          Serial.println("Bad user state; re-init");
          g_nv_state.init();
          write_nvm();
      }
  }

  ir_setup();
  
  bt_setup();
}

//
// VFD tube
//
const uint8_t map_seg_to_bit[7] = {
  5, 6, 1, 0, 4, 3, 2
};

const uint8_t NUM_DIGITS = 12;
const uint8_t TUBE_PER_DIG = 2;
uint8_t sdi_d[NUM_DIGITS * TUBE_PER_DIG] = {0};

const int TUBE(uint8_t digit, uint8_t lo_hi) {
    return (digit + g_nv_state.st.rotation) % NUM_DIGITS * TUBE_PER_DIG + lo_hi;
}

bool do_ir()
{
  ir_cmd_t ir_cmd;
  bool ir_rep;                  // repeated-key flag. Usually ignored.
  
  if (ir_check(ir_cmd, ir_rep)) {
      const int prev_ir_nums_size = ir_nums.size(); // track number entry
      
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

          case IR_200:
              if (!ir_rep && ir_nums.size() > 0) {
                  int val = 0;
                  for (int i = 0; i < ir_nums.size(); i ++) {
                      val = val*10 + ir_nums[i];
                  }
                  set_brightness(val);
              }
              break;

          case IR_VOLUP:
          case IR_VOLDOWN:
              if (true) { // allow repeated
                  int dir = (ir_cmd == IR_VOLDOWN) ? -1 : +1;
                  set_brightness(g_nv_state.st.brightness + dir);
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
                  printDateTime(new_local, tcr->abbrev);
                  ds3231_set(new_utc);
                  g_time_valid = true;
                  time_to_vfd();
              }
              break;

          case IR_CH:
              if (!ir_rep) {
                  time_t utc = ds3231_get_unixtime();
                  time_t local = myTZ.toLocal(utc, &tcr);
                  Serial.print("Get time: ");
                  if (!g_time_valid) Serial.print(" [INVALID] ");
                  printDateTime(local, tcr->abbrev);            
              }
              break;
              
          case IR_CHDOWN:
              if (!ir_rep) {
                  uint8_t *b = g_nv_state.get_packed_bytes();
                  ds3231_getUserBytes(b, g_nv_state.get_packed_size());
                  Serial.print(b[0], HEX); 
                  Serial.print(b[1], HEX); 
                  Serial.print(b[2], HEX); 
                  Serial.print(b[3], HEX); 
                  Serial.print(b[4], HEX); 
                  if (g_nv_state.validate_checksum()) {
                      Serial.print(". Get valid user state; b "); Serial.println(g_nv_state.st.brightness);
                      Serial.print("   rot "); Serial.println(g_nv_state.st.rotation);
                  }
                  else {
                      Serial.println(" Bad user state");
                  }
              }
              break;
              
          case IR_FF:
          case IR_REW:
              if (!ir_rep) {
                  uint8_t dir = (ir_cmd == IR_REW) ? 11 : 1;
                  set_rotation((g_nv_state.st.rotation + dir) % NUM_DIGITS);
              }
              break;
              
          case IR_EQ:
              if (!ir_rep && ir_nums.size() > 0) {
                  int val = 0;
                  for (int i = 0; i < ir_nums.size(); i ++) {
                      val = val*10 + ir_nums[i];
                  }
                  g_test_mode = val;
                  Serial.print("Set test mode "); Serial.println(g_test_mode);
              }
              break;
      }

      // if a key other than a digit was entered, clear the accumulated number.
      if (!ir_rep && ir_nums.size() == prev_ir_nums_size) {
          ir_nums.clear();
      }

      return true;
  }

  return false;
}

void set_brightness(int b_) {
    uint8_t b = max(0, min(b_, 255));
    analogWrite(PIN_OE_, gamma8(255 - b));
    Serial.print("Set brightness: "); Serial.println(b);
    if (g_nv_state.st.brightness != b) {
        g_nv_state.st.brightness = b;
        write_nvm();
    }
}

void set_rotation(int r_) {
    uint8_t r = max(0, min(r_, NUM_DIGITS-1));
    Serial.print("Set rotation: "); Serial.println(r);
    if (g_nv_state.st.rotation != r) {
        g_nv_state.st.rotation = r;
        write_nvm();
    }
}

void write_nvm() {
    ds3231_setUserBytes(g_nv_state.get_packed_bytes(), g_nv_state.get_packed_size());
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
    if (!g_first_vfd_update) {
        set_brightness(g_nv_state.st.brightness);
        g_first_vfd_update = true;
    }
}

void time_to_vfd()
{
    const time_t utc_time = ds3231_get_unixtime();
    const time_t new_local_time = myTZ.toLocal(utc_time, &tcr);
    if (new_local_time == g_local_time) {
        return;
    }
    g_local_time = new_local_time;

    memset(sdi_d, 0, sizeof(sdi_d));

    int hr_tube0 = TUBE(hour(g_local_time) % 12, 0);
    int hr_tube1 = TUBE(hour(g_local_time) % 12, 1);
    int min_tube0 = TUBE(minute(g_local_time) / 5, 0);
    int min_tube1 = TUBE(minute(g_local_time) / 5, 1);

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
    int minutes_extra = minute(g_local_time) % 5;
    if (minutes_extra == 0) {
        minutes_extra = 5;
    }
    for (int i = 2; i < 2 + minutes_extra; i ++) {
        sdi_d[min_tube1] |= 1<<map_seg_to_bit[i];
    }

    // second: flash remaining 10 segments' lowest
    for (int i = 0; i < NUM_DIGITS; i ++) {
        int sec_tube = TUBE(i, 0);
        if (sec_tube != hr_tube0 && sec_tube != min_tube0) {
            sdi_d[sec_tube] |= (second(g_local_time) & 1) << map_seg_to_bit[0];
        }
    }
    digitalWrite(PIN_LED, (second(g_local_time) & 1));
    update_vfd();
}

void do_test_vfd() {
    static int16_t cnt = 0;
    uint8_t digit = cnt/14;
    uint8_t lo_hi = (cnt/7) % 2;
    uint8_t seg = cnt % 7;
    memset(sdi_d, 0, sizeof(sdi_d));
    
    for (uint8_t tube = 0 ; tube < 6; tube ++) {
        sdi_d[tube] = (cnt&1) ? (1<<map_seg_to_bit[tube]) : ~(1<<map_seg_to_bit[tube]);
    }
    update_vfd();
    
    cnt=(cnt+1)%28;
    digitalWrite(PIN_LED, cnt&1);
}

elapsedMillis vfd_timer;

void loop() {
  bool has_command = do_ir();

  String bt_cmd;
  if (bt_loop(bt_cmd)) {
      if (bt_cmd.startsWith("b ")) {
          int b = atoi(bt_cmd.substring(bt_cmd.indexOf(' ')).c_str());
          Serial.print("bt brightness:"); Serial.println(b);
          set_brightness(b);
      }
      else if (bt_cmd.startsWith("st ")) { // local "ISO 8601" format e.g. 2020-06-25T15:29:37
          time_t local_time = datetime_parse(bt_cmd.substring(bt_cmd.indexOf(' ') + 1));
          time_t new_utc = myTZ.toUTC(local_time);

          Serial.print("bt set time: ");
          printDateTime(local_time, tcr->abbrev);
          ds3231_set(new_utc);
          g_time_valid = true;
      }
      else if (bt_cmd.startsWith("r ")) {
          int rot = atoi(bt_cmd.substring(bt_cmd.indexOf(' ')).c_str());
          Serial.print("bt rot:"); Serial.println(rot);
          set_rotation(rot);
          write_nvm();
      }
      has_command = true;
  }

  if (g_test_mode == 0 && g_time_valid && (has_command || vfd_timer >= 200)) {
      time_to_vfd();
      vfd_timer = 0;
  }
  else if (has_command || vfd_timer >= 500) {
      const time_t utc_time = ds3231_get_unixtime(); // FIXME testing I2C
      do_test_vfd();
      vfd_timer = 0;
  }

}

void printDateTime(time_t t, const char *tz) {
  char buf[32];

  char m[4]; // temporary storage for month string (DateStrings.cpp uses shared
             // buffer)
  strlcpy(m, monthShortStr(month(t)), 4);
  sprintf(buf, "%.2d:%.2d:%.2d %s %.2d %s %d %s", hour(t), minute(t), second(t),
          dayShortStr(weekday(t)), day(t), m, year(t), tz);
  Serial.println(buf);
}
