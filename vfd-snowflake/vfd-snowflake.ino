
// Elegoo Nano (ATMega328 old bootloader)


#include "TimerHelpers.h"

const int PAD_SD1=3, PAD_SCLK=2, PAD_RCLK=4, PAD_SD2=5;
const int PAD_FIL0=9, PAD_FIL1=10;  // filament "A/C" waveform

enum { SEG_L, SEG_M, SEG_H};
static const byte tube_shift[4] = {13, 10, 5, 2};

static const byte vfd_seg_map[4][3] = { // seg index to bit mask
  { 4,1,2 },
  { 4,1,2 },
  { 4,1,2 },
  { 4,2,1 },  // BUG
};

void setup() {
  Serial.begin(9600);
  pinMode(PAD_SD1, OUTPUT);
  pinMode(PAD_SD2, OUTPUT);
  pinMode(PAD_SCLK, OUTPUT);
  pinMode(PAD_RCLK, OUTPUT);
  pinMode(PAD_FIL0, OUTPUT);
  pinMode(PAD_FIL1, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  // set up Timer 0
  TCNT0 = 0;
  OCR1A = 128;
  OCR1B = 128;
  // Mode 1: CTC, top = OCR1A
  Timer1::setMode (1, Timer1::PRESCALE_64, Timer1::CLEAR_A_ON_COMPARE | Timer1::SET_B_ON_COMPARE);
 
}

void shiftOut(uint16_t d)
{
  for (int i = 0; i < 16; i ++) {
    digitalWrite(PAD_SD1, (d>>i) & 1);
    digitalWrite(PAD_SD2, (d>>i) & 1);
    digitalWrite(PAD_SCLK, 1);
    digitalWrite(PAD_SCLK, 0);
  }
  digitalWrite(PAD_RCLK, 1);
  digitalWrite(PAD_RCLK, 0);
}
void loop() {
  // put your main code here, to run repeatedly:
  #if 1
  const int D = 100;
  for (int t = 0; t < 4; t++) {
   digitalWrite(LED_BUILTIN, t&1);
   shiftOut(vfd_seg_map[t][SEG_L] << tube_shift[t] );
   delay(D);
   shiftOut((vfd_seg_map[t][SEG_L]|vfd_seg_map[t][SEG_M]) << tube_shift[t]);
   delay(D);
   shiftOut((vfd_seg_map[t][SEG_L]|vfd_seg_map[t][SEG_M]|vfd_seg_map[t][SEG_H]) << tube_shift[t]);
   delay(D);
   shiftOut((vfd_seg_map[t][SEG_M]|vfd_seg_map[t][SEG_H]) << tube_shift[t]);
   delay(D);
   shiftOut(vfd_seg_map[t][SEG_H] << tube_shift[t]);
   delay(D);
  }
   #endif

   shiftOut(0x0000);
   delay(D);
}
