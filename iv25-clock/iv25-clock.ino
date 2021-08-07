#include <TimeLib.h>
#include <Timezone_Generic.h>

//static const int PIN_CLK = 12;
//static const int PIN_SDI = 11;
//static const int PIN_STROBE = 10;
//static const int PIN_OE_ = 9;
//static const int PIN_LED = LED_BUILTIN;

static const int PIN_CLK = 6;
static const int PIN_SDI = 7;
static const int PIN_STROBE = 8;
static const int PIN_OE_ = 9;
static const int PIN_LED = 17;  // TX

extern bool ds3231_setup(int clockSqwPin, void (*isr)(void));

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
      time_t local_time = datetime_parse("2021-08-07T14:02:00");
      time_t new_utc = myTZ.toUTC(local_time);
       Serial.print("Set time: ");
        printDateTime(new_utc, tcr->abbrev);
  
    ds3231_set(new_utc);
  }
}

const uint8_t map_seg_to_bit[7] = {
  5, 6, 1, 0, 4, 3, 2
};

uint16_t cnt = 0;
void loop() {
   uint16_t seg_bit0 = ((cnt/7) == 0) ? (1 << map_seg_to_bit[cnt % 7]) : 0;
   uint16_t seg_bit1 = ((cnt/7) == 1) ? (1 << map_seg_to_bit[cnt % 7]) : 0;
   uint16_t sdi_d = seg_bit0 | (seg_bit1 << 8);
     for (int i = 0; i < 16; i ++) {
      digitalWrite(PIN_SDI, (sdi_d >> (15 - i)) & 0x1);
      digitalWrite(PIN_CLK, 1);
      digitalWrite(PIN_CLK, 0);
   }
   digitalWrite(PIN_STROBE, 1);
   digitalWrite(PIN_STROBE, 0);
   digitalWrite(PIN_OE_, 0);

   
   delay(250);
   cnt=(cnt+1)%14;
   digitalWrite(PIN_LED, cnt&1);
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
