#include <SPI.h>
#include "Adafruit_TPA2016.h"

Adafruit_TPA2016 audioamp = Adafruit_TPA2016();

static const int PIN_CLK = 2;
static const int PIN_SDI = 0;
static const int PIN_STROBE = 1;
static const int PIN_PWM = 9;

uint8_t sdi_d[2] = {0};

void update_shift()
{
  for (int i = 0; i < 16; i ++) {
    digitalWrite(PIN_SDI, (sdi_d[i/8] >> (7 - (i%8))) & 0x1);
    digitalWrite(PIN_CLK, 1);
    digitalWrite(PIN_CLK, 0);
  }
  digitalWrite(PIN_STROBE, 1);
  digitalWrite(PIN_STROBE, 0);
}

void setup() {
    Serial.begin(9600);
 pinMode(PIN_CLK, OUTPUT);
  pinMode(PIN_SDI, OUTPUT);
  pinMode(PIN_STROBE, OUTPUT);
  digitalWrite(PIN_CLK, 0);
  digitalWrite(PIN_SDI, 0);
  digitalWrite(PIN_STROBE, 0);

  pinMode(PIN_PWM, OUTPUT);
  analogWriteFrequency(PIN_PWM, 500);
   analogWrite(PIN_PWM, 0);

audioamp.begin();
  Serial.println("Start");
}

static uint8_t cnt = 0;
void loop() {
if (Serial.available() > 0) {
    uint8_t val = Serial.readStringUntil('\n').toInt();
   Serial.print("got "); Serial.println(val);
    analogWrite(PIN_PWM, val);
//    sdi_d[0] = ~val;
//    update_shift();
  }
  unsigned led = (cnt == 0);
 sdi_d[0] = ~(cnt | (led << 7));
  sdi_d[0] = ~(cnt | (cnt << 4));

 cnt = (cnt+1) % 10;
 
 update_shift();
 delay(400);

// audioamp.setAGCCompression(TPA2016_AGC_OFF);
//  audioamp.setGain(cnt);
//delay(500);
//  int g = audioamp.getGain();
//  if (g == cnt) { Serial.println("pass"); }
//  else { Serial.println("FAIL"); }
//  cnt = (cnt + 1) % 28;
//  
}
