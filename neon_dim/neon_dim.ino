//
// Teensy 3.2
//

#include <elapsedMillis.h>

#if 0
#define PIN_DAC_CS   5
#define PIN_DAC_SCLK 6
#define PIN_DAC_SDI  7

// MCP4901
void MCP4901_dac_write(byte value)
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
#endif


#define PIN_DAC_CLK 10
#define PIN_DAC_DATA 11
#define PIN_DAC_LOAD 12

void tlc5620_dac_write(byte which, byte value)
{
  enum {ADDR=9, RNG=8};
  const uint16_t v16 = (which<<ADDR) | (0<<RNG) | value;
  for (byte b = 0; b < 11; b ++) {
      digitalWrite(PIN_DAC_CLK, 0);
      digitalWrite(PIN_DAC_DATA, ~(v16 >> (10 - b)) & 1);
      digitalWrite(PIN_DAC_CLK, 1);
    }
    digitalWrite(PIN_DAC_LOAD, 1);
    digitalWrite(PIN_DAC_LOAD, 0);
}

void setup() {
  Serial.begin(9600);
  
   pinMode(LED_BUILTIN, OUTPUT);
   #if 0
   digitalWrite(PIN_DAC_CS, HIGH);
   pinMode(PIN_DAC_CS, OUTPUT);
   pinMode(PIN_DAC_SDI, OUTPUT);
   digitalWrite(PIN_DAC_CS,HIGH);
   pinMode(PIN_DAC_SCLK, OUTPUT); 
   #endif

   digitalWrite(PIN_DAC_LOAD, 0);
   digitalWrite(PIN_DAC_CLK, 1);
   pinMode(PIN_DAC_LOAD, OUTPUT);
   pinMode(PIN_DAC_CLK, OUTPUT);
   pinMode(PIN_DAC_DATA, OUTPUT);

  
}

const byte DAC_MIN = 5;
const byte DAC_MAX = 200;
byte val = DAC_MIN;
byte dir = 0;

elapsedMillis capTimer;

void loop() {

  tlc5620_dac_write(0, val);
#if 0
  val += dir ? -5 : 5;
  if (val < DAC_MIN) {
    val = DAC_MIN;
    dir = 0;
  }
  if (val > DAC_MAX) {
    val = DAC_MAX;
    dir = 1;
  }
  delay(10);
  digitalWrite(LED_BUILTIN, dir);
#endif

  if (capTimer > 100) {
    capTimer=0;
    digitalWrite(LED_BUILTIN,0);
    //long v1 = capSensor1.capacitiveSensor(30);
    long v1 = touchRead(22);
    long v2 = touchRead(23);
    //Serial.print(v1); Serial.print("  ");
    //Serial.println(v2);
    if (v1 > 2000 and val <= 250) { val += 5; digitalWrite(LED_BUILTIN,1); }
    if (v2 > 2000 and val >= 5) { val -= 5; digitalWrite(LED_BUILTIN,1); }
    
  }
}
