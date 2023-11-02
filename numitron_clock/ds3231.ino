/* Example implementation of an alarm using DS3231
 *
 * VCC and GND of RTC should be connected to some power source
 * SDA, SCL of RTC should be connected to SDA, SCL of arduino
 * SQW should be connected to CLOCK_INTERRUPT_PIN
 * CLOCK_INTERRUPT_PIN needs to work with interrupts
 */

#include <RTClib.h>
// #include <Wire.h>

static RTC_DS3231 rtc;

bool ds3231_setup() {
  bool rc = true;
  // initializing the rtc
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC!");
    rc = false;
  }

  if (rtc.lostPower()) {
    Serial.println("Lost Power");
    // this will adjust to the date and time at compilation
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    rc = false;
  }

  // we don't need the 32K Pin, so disable it
  rtc.disable32K();

  // set alarm 1, 2 flag to false (so alarm 1, 2 didn't happen so far)
  // if not done, this easily leads to problems, as both register aren't reset
  // on reboot/recompile
  rtc.clearAlarm(1);
  rtc.clearAlarm(2);
  // turn off alarms (in case it isn't off already)
  // again, this isn't done at reboot, so a previously set alarm could easily go
  // overlooked
  rtc.disableAlarm(1);
  rtc.disableAlarm(2);

  // attachInterrupt(digitalPinToInterrupt(clockSqwPin), isr, RISING);
  // rtc.writeSqwPinMode(DS3231_SquareWave1Hz);

  return rc;
}

time_t ds3231_get_unixtime(void) { return rtc.now().unixtime(); }

void ds3231_set(time_t unix_time_utc) {
  DateTime td(unix_time_utc);
  rtc.adjust(td);
}

time_t datetime_parse(const String &s) {
    DateTime dt(s.c_str());
    return dt.unixtime();
}
bool ds3231_setUserBytes(const uint8_t *bytes, int num_bytes)
{
  Serial.print("setUserbytes "); Serial.println(num_bytes);
  return rtc.setUserBytes(bytes, num_bytes);
}
bool ds3231_getUserBytes(uint8_t *bytes, int num_bytes)
{
  return rtc.getUserBytes(bytes, num_bytes);
}
