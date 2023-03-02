/*
 * segments: 
 *    --a--
 *   |    |
 *   f    b
 *   |    |
 *   --g---
 *   |    |
 *   e    c
 *   |    |
 *   +-d--+
 *   
 * Connector:  1  2  3 / 4 5 6 7 8 9 10 11
 *            d1 d2 d3 / g f a b e d c  dp
 *             digit   /     segment        
 *    pins:   2  3  4  / 12 11 10 9 8 7 6  5
 */

#include "elapsedMillis.h"

#define A 16
#define B 8
#define C 1
#define D 2
#define E 4
#define F 32
#define G 64
#define PIN_SEGBASE 6
#define PIN_DP 5
#define PIN_DIGBASE 2
#define PIN_CURR A7

const uint16_t segmentMask[] = {
  A|B|C|D|E|F, // 0
  B|C,         // 1
  A|B|G|E|D,   // 2
  A|B|G|C|D,   // 3
  F|G|B|C,     // 4
  A|F|G|C|D,   // 5
  A|F|G|E|C|D, // 6
  A|B|C,       // 7
  A|B|C|D|E|F|G, // 8
  A|B|C|F|G,     // 9
};

 int c_digits[3] = {0,0,0};
 int c_dp = 0;  // -1, 0,1,2  (-1 means none)
 
 void setup() {
  // put your setup code here, to run once:
  for (int i = 0; i < 7; i ++) {
     pinMode(PIN_SEGBASE+i, OUTPUT);
     digitalWrite(PIN_SEGBASE+i,0);
  }
  for (int i = 0; i < 3; i++) {
    pinMode(PIN_DIGBASE+i, OUTPUT);
    digitalWrite(PIN_DIGBASE+i,0);
  }
  pinMode(PIN_DP, OUTPUT);  
  digitalWrite(PIN_DP,0);

  pinMode(PIN_CURR, INPUT);
  Serial.begin(38400);
}

void disable_digits()
{
    for (int i=0; i<3; i++) {
      digitalWrite(PIN_DIGBASE+i, 0);
    }
}

void enable_digit(int dig) {
   digitalWrite(PIN_DIGBASE+dig, 1);
}
void set_digit(int val)
{
  for (int seg = 0; seg < 7; seg ++) {
    digitalWrite(PIN_SEGBASE+seg, ((segmentMask[val] >> seg) & 1) ? 1 : 0);
  }
}
void set_dp(int val)
{
  digitalWrite(PIN_DP, val);
}
void set_seg(int s)
{
  for (int seg = 0; seg < 7; seg ++) {
    digitalWrite(PIN_SEGBASE+seg, (s==seg) ? 1 : 0);
  }
}

// https://github.com/thuaung30/TA12-100-current-sensor-/blob/master/TA10_sensor_code.ino

 void update_current() 
 {
   static float prev_v = 0;
   static float resistance = 200;  
   float nVPP = getVPP();
   if (nVPP == prev_v) {
    return;
   }
   prev_v = nVPP;
   
   /*
   Use Ohms law to calculate current across resistor
   and express in A 
   */
   
   float nCurrThruResistorPP = (nVPP/resistance);
   
   /* 
   Use Formula for SINE wave to convert
   to RMS 
   */
   
   float nCurrThruResistorRMS = nCurrThruResistorPP * 0.707;
   
   /* 
   Current Transformer Ratio is 1000:1...
   
   Therefore current through 200 ohm resistor
   is multiplied by 1000 to get input current
   */
   
   float nCurrentThruWire = nCurrThruResistorRMS * 1000;

  #if 0
   Serial.print("Volts Peak : ");
   Serial.println(nVPP,3);
 
   
   Serial.print("Current Through Resistor (Peak) : ");
   Serial.print(nCurrThruResistorPP,3);
   Serial.println(" A Peak to Peak");
   
   Serial.print("Current Through Resistor (RMS) : ");
   Serial.print(nCurrThruResistorRMS,3);
   Serial.println(" A RMS");
   
   Serial.print("Current Through Wire : ");
   Serial.print(nCurrentThruWire,3);
   Serial.println(" A RMS");
   
   Serial.println();
  #endif
   int norm_c;
   if (nCurrentThruWire > 999) {
    norm_c = 999;
   }
   
   if (nCurrentThruWire > 99) {
    norm_c = int(nCurrentThruWire + 0.5);
    c_dp = 2;
   }
   else if (nCurrentThruWire > 9) {
    norm_c = int(nCurrentThruWire * 10.0 + 0.5);
    c_dp = 1;
   }
   else if (nCurrentThruWire >= 1) {
    norm_c = int(nCurrentThruWire * 100.0 + 0.5);
    c_dp = 0;
   }
   else {
    norm_c = int(nCurrentThruWire * 1000.0 + 0.5);
    c_dp = -1;
   }
   
   c_digits[0] = norm_c/100;
   c_digits[1] = norm_c%100/10;
   c_digits[2] = norm_c%10;
 }


 int numSamps = 0;
 elapsedMillis sampTime;
 
float getVPP()
{
  static int maxValue = 0, minValue = 1023*6;
  static int prevMax[2] = {0,0};
  static int prevMin[2] = {0,0};
  static int prevA2D[5];
  static float lastValueV = 0.0; // volts

  if (sampTime >= 1) {
      int a2d = analogRead(PIN_CURR);
      for (int i = 0; i < 4; i++)
        prevA2D[i] = prevA2D[i+1];
      prevA2D[4] = a2d;
      //int avgScaled = prevA2D[0] * -1 + prevA2D[1] * 2 + prevA2D[2] * 4 + prevA2D[3] * 2 + prevA2D[4] * -1; // scale of 6
      int avgScaled = a2d*6;
      //Serial.println(a2d);
      maxValue = max(maxValue, avgScaled);
      minValue = min(minValue, avgScaled);
      sampTime = 0;

      if (numSamps++ >= 100) { // update and restart every 0.1 sec
         // simple filter averaging 3 values, total weight 6.
         //lastValueV = (3*(prevMax[0] - prevMin[0]) + 2*(prevMax[1] - prevMin[1]) + (maxValue - minValue)) * (5.0 / 2.0 / 6.0  / 6.0 / 1024.0);
         lastValueV = (maxValue - minValue) * (5.0 / 2.0 / 6.0 / 1024.0);
         prevMax[0] = prevMax[1];
         prevMin[0] = prevMin[1];
         prevMax[1] = maxValue;
         prevMin[1] = minValue;
         //float v = (maxValue - minValue) * (5.0 / 2.0 / 6.0 / 1024.0);
         //Serial.print("a2d/Min/Max/v "); Serial.print(avgScaled); Serial.print(" "); Serial.print(minValue); Serial.print(" "); Serial.print(maxValue);Serial.print(" "); Serial.println(v,3);

         numSamps = 0;
         maxValue = 0;
         minValue = 1023*6;
       }
  }
       
   return lastValueV;
 }
 
elapsedMillis m;
int v=0;
int d=0;
void loop() {
  update_current();
  
  disable_digits();
  set_digit(c_digits[d]);
  set_dp(c_dp == d);
  enable_digit(d); 

  d=(d+1)%3;  
}
