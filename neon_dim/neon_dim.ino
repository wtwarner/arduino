#define PIN_DAC_CS   5
#define PIN_DAC_SCLK 6
#define PIN_DAC_SDI  7

// MCP4901
void dac_write(byte value)
{
  enum {WR_N=15, BUF=14, GA_N=13, STDN_N=12};
    const uint16_t v16 = (0<<WR_N) | (0<<BUF) | (1<<GA_N) | (1<<STDN_N) | ((uint16_t)value << 4);

    digitalWrite(PIN_DAC_CS, LOW);
    for (byte b = 0; b < 16; b ++) {
      digitalWrite(PIN_DAC_SCLK, LOW);
      digitalWrite(PIN_DAC_SDI, (v16 >> (15 - b)) & 1);
      digitalWrite(PIN_DAC_SCLK, HIGH);
    }
    digitalWrite(PIN_DAC_CS, HIGH);
}

void setup() {
   pinMode(LED_BUILTIN, OUTPUT);
   digitalWrite(PIN_DAC_CS, HIGH);
   pinMode(PIN_DAC_CS, OUTPUT);
   pinMode(PIN_DAC_SDI, OUTPUT);
   digitalWrite(PIN_DAC_CS,HIGH);
   pinMode(PIN_DAC_SCLK, OUTPUT); 
}

const byte DAC_MIN = 50;
const byte DAC_MAX = 250;
byte val = DAC_MIN;
byte dir = 0;

void loop() {
  dac_write(DAC_MAX);

  dac_write(val);
  val += dir ? -5 : 5;
  if (val < DAC_MIN) {
    val = DAC_MIN;
    dir = 0;
  }
  if (val > DAC_MAX) {
    val = DAC_MAX;
    dir = 1;
  }
  delay(50);
  digitalWrite(LED_BUILTIN, dir);
}
