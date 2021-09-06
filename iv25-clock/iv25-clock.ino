#include <TimeLib.h>
#include <Timezone_Generic.h>
#include "ir.h"

static const int PIN_CLK = 6;
static const int PIN_SDI = 7;
static const int PIN_STROBE = 8;
static const int PIN_OE_ = 9;
static const int PIN_LED = 17;  // TX

extern bool ds3231_setup();
extern time_t ds3231_get_unixtime(void); 

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

  ir_setup();
  bt_setup();
}

const uint8_t map_seg_to_bit[7] = {
  5, 6, 1, 0, 4, 3, 2
};

const uint8_t NUM_DIGITS = 3;
const uint8_t TUBE_PER_DIG = 2;
uint16_t cnt = 0;
uint8_t sdi_d[NUM_DIGITS * TUBE_PER_DIG] = {0};
const int TUBE(uint8_t digit, uint8_t lo_hi) {
    return digit * TUBE_PER_DIG + lo_hi;
}

void do_ir()
{
  ir_cmd_t ir_cmd;
  bool ir_rep;
  if (ir_check(ir_cmd, ir_rep)) {
      switch (ir_cmd) {
          case IR_FF:
              if (!ir_rep)
                  ;
              break;
          case IR_FUNCSTOP:
              if (ir_rep) {
                  ; // nop
              }
              
              break;

          case IR_VOLUP:

              break;
          case IR_VOLDOWN:

              break;
      }
  }
}

void do_test_vfd() {
  uint8_t digit = cnt/14;
  uint8_t lo_hi = (cnt/7) % 2;
  uint8_t seg = cnt % 7;
  memset(sdi_d, 0, sizeof(sdi_d));
  
   //sdi_d[TUBE(digit,lo_hi)] |= 1 << map_seg_to_bit[seg];
  for (uint8_t tube = 0 ; tube < 6; tube ++) {
   sdi_d[tube] = (cnt&1) ? (1<<map_seg_to_bit[tube]) : ~(1<<map_seg_to_bit[tube]);
  }
   
   for (int i = 0; i < NUM_DIGITS * TUBE_PER_DIG * 8; i ++) {
      digitalWrite(PIN_SDI, (sdi_d[NUM_DIGITS * TUBE_PER_DIG - 1 - i/8] >> (7 - (i%8))) & 0x1);
      digitalWrite(PIN_CLK, 1);
      digitalWrite(PIN_CLK, 0);
   }
   digitalWrite(PIN_STROBE, 1);
   digitalWrite(PIN_STROBE, 0);
   digitalWrite(PIN_OE_, 0);


   cnt=(cnt+1)%28;
   digitalWrite(PIN_LED, cnt&1);
}

long last_ms = 0;
void loop() {
  do_ir();


  if (millis() > (last_ms + 250)) {
    do_test_vfd();
    last_ms = millis();
  }

 String bt_cmd;
 if (bt_loop(bt_cmd)) {
    if (bt_cmd.startsWith("b ")) {
          int pwm = atoi(bt_cmd.substring(bt_cmd.indexOf(' ')).c_str());
          Serial.print("pwm:"); Serial.println(pwm);
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
