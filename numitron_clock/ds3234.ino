#include <SPI.h>
#include <RtcDS3234.h>

static const int PIN_CLK	= 29;
static const int PIN_MISO = 30;
static const int PIN_MOSI = 31;
static const int PIN_CS	= 32;

// assumes MSBFIRST + SPI_MODE1 (CPOL=0, CPHA=1)
// data output on rising SCLK, sampled on falling SCLK
class SoftSPI {
  public:
  SoftSPI(int cs_pin, int mosi_pin, int miso_pin, int clk_pin):
  cs_pin(cs_pin),
  mosi_pin(mosi_pin),
  miso_pin(miso_pin),
  clk_pin(clk_pin)
  {
    digitalWrite(cs_pin, 1);
    pinMode(cs_pin, OUTPUT);
    digitalWrite(mosi_pin, 0);
    pinMode(mosi_pin, OUTPUT);
    pinMode(miso_pin, INPUT);
    digitalWrite(clk_pin, 0);
    pinMode(clk_pin, OUTPUT);
  }

  void beginTransaction(const SPISettings &s)
  {}

  uint8_t transfer(uint8_t td)
  {
    uint8_t rd = 0;
    for (int i = 0; i < 8; i ++) {
      digitalWrite(clk_pin, 1);
      digitalWrite(mosi_pin, (td & 0x80) ? 1 : 0);
      td = td << 1;

      digitalWrite(clk_pin, 0);
      rd = rd << 1;
      rd |= digitalRead(miso_pin);
    }
    return rd;
  }

  void endTransaction()
  {}

  private:
  int cs_pin, mosi_pin, miso_pin, clk_pin;
};

static SoftSPI softSPI(PIN_CS, PIN_MOSI, PIN_MISO, PIN_CLK);
static RtcDS3234<SoftSPI> rtc3234(softSPI, PIN_CS);

bool ds3234_setup() {
  bool rc = true;

  rtc3234.Begin();

  if (!rtc3234.IsDateTimeValid()) {
    Serial.println("Lost Power");
    rc = false;
  }
  if (!rtc3234.GetIsRunning()) {
    Serial.println("Not running");
    rc = false;
  }

  // we don't need the 32K/INT Pin, so disable it
  rtc3234.Enable32kHzPin(false);
  rtc3234.SetSquareWavePin(DS3234SquareWavePin_ModeNone);

  return rc;
}

time_t ds3234_get_unixtime(void) {
  return rtc3234.GetDateTime().Unix64Time();
}

void ds3234_set(time_t unix_time_utc) {
  RtcDateTime dt;
  dt.InitWithUnix64Time(unix_time_utc);
  rtc3234.SetDateTime(dt);
}

// local "ISO 8601" format e.g. 2020-06-25T15:29:37
time_t ds3234_datetime_parse(const String &s) {
    RtcDateTime dt;
    dt.InitWithDateTimeFormatString("YYYY-MM-DDThh:mm::ss", s.c_str());
    return dt.Unix64Time();
}

bool ds3234_setUserBytes(const uint8_t *bytes, int num_bytes)
{
  Serial.print("setUserbytes "); Serial.println(num_bytes);
  return rtc3234.SetMemory(0, bytes, num_bytes) == num_bytes;
}

bool ds3234_getUserBytes(uint8_t *bytes, int num_bytes)
{
  return rtc3234.GetMemory(0, bytes, num_bytes) == num_bytes;
}
