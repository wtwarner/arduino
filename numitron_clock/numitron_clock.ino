//
// Teensy 3.2
//

#include <elapsedMillis.h>
#include "LiquidCrystal.h"



#define PIN_DAC_CLK 10
#define PIN_DAC_DATA 11
#define PIN_DAC_LOAD 12
const int PIN_TOUCH[] = {22,23};

const int PIN_LCD_RS = 0;
const int PIN_LCD_EN = 1;
const int PIN_LCD_D4 = 2;
const int PIN_LCD_D5 = 3;
const int PIN_LCD_D6 = 4;
const int PIN_LCD_D7 = 5;
const int PIN_LCD_BL_PWM = 20;

const int PIN_BULB_PWM = 21;

const int PIN_TPIC_SCLK = 6;
const int PIN_TPIC_RCLK = 7;
const int PIN_TPIC_SDI = 8;
const int PIN_TPIC_OE_ = 9;

LiquidCrystal lcd(PIN_LCD_RS, PIN_LCD_EN, PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7);

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

void setup() {
   Serial.begin(9600);
   Serial.println("start");
  
   pinMode(LED_BUILTIN, OUTPUT);
  
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

  // set up the LCD's number of columns and rows:
  //lcd.setInvertPins(true);
  digitalWrite(PIN_LCD_BL_PWM, 0);
  pinMode(PIN_LCD_BL_PWM, OUTPUT);
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("hello, world!");


}

void update_tpic(byte sdi[1])
{
  for (int i = 0; i < 8; i ++) {
    digitalWrite(PIN_TPIC_SDI, (sdi[i / 8] >> (7 - (i % 8))) & 0x1);
    digitalWrite(PIN_TPIC_SCLK, 1);
    digitalWrite(PIN_TPIC_SCLK, 0);
  }
  digitalWrite(PIN_TPIC_RCLK, 1);
  digitalWrite(PIN_TPIC_RCLK, 0);
  digitalWrite(PIN_TPIC_OE_, 0);
}


const byte DAC_MIN = 5;
const byte DAC_MAX = 250;
byte val = 150;
byte dir = 0;

elapsedMillis capTimer;

byte tpic_data[1] = {0xff};

void loop() {

  tlc5620_dac_write(0, val);

  if (capTimer > 100) {
    capTimer=0;
    digitalWrite(LED_BUILTIN,0);
    //long v1 = capSensor1.capacitiveSensor(30);
    long v1 = touchRead(PIN_TOUCH[0]);
    long v2 = touchRead(PIN_TOUCH[1]);
    //Serial.print(v1); Serial.print("  ");
    //Serial.println(v2);
    bool update = false;
    if (v1 > 2000 and val <= 250) { val += 5; update = true; }
    if (v2 > 2000 and val >= 5) { val -= 5; update = true; }
    if (update) { 
      digitalWrite(LED_BUILTIN,1);
      lcd.clear();
      lcd.print(val);
      update_tpic(tpic_data);
      //tpic_data[0] ^= 0x3;
    }
  }
  #if 1
  if (dir==0) {
     if (val > DAC_MIN) { val -= 5; }
     else { dir = 1 - dir; }
  }
  else {
    if (val < DAC_MAX) { val += 5; }
    else { dir = 1 - dir;}
  }
  lcd.clear();
  lcd.print(val);
  update_tpic(tpic_data);
  analogWrite(PIN_LCD_BL_PWM, val);
  analogWrite(PIN_BULB_PWM, val);
  #endif
  delay(100);
}
