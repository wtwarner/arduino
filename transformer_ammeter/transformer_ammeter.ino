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

uint16_t segmentMask[] = {
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

  pinMode(A7, INPUT);
}

int cnt = 0;

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
void set_seg(int s)
{
  for (int seg = 0; seg < 7; seg ++) {
    digitalWrite(PIN_SEGBASE+seg, (s==seg) ? 1 : 0);
  }
}

elapsedMillis m;
int v=0;
int d=0;
void loop() {
  
  disable_digits();
   
  set_digit((v+d) % 10);
  enable_digit(d); 

  d=(d+1)%3;
  if (m > 500) {
    v = (v+1)%10;
    m=0;
  }
  
}
