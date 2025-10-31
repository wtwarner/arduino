
// Elegoo Nano (ATMega328 old bootloader)


#include "TimerHelpers.h"

const int PAD_SD=3, PAD_SCLK=2, PAD_RCLK=4;
const int PAD_FIL0=9, PAD_FIL1=10;  // filament "A/C" waveform

void setup() {
  Serial.begin(9600);
  pinMode(PAD_SD, OUTPUT);
  pinMode(PAD_SCLK, OUTPUT);
  pinMode(PAD_RCLK, OUTPUT);
  pinMode(PAD_FIL0, OUTPUT);
  pinMode(PAD_FIL1, OUTPUT);

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
    digitalWrite(PAD_SD, (d>>i) & 1);
    digitalWrite(PAD_SCLK, 1);
    digitalWrite(PAD_SCLK, 0);
  }
  digitalWrite(PAD_RCLK, 1);
  digitalWrite(PAD_RCLK, 0);
}
void loop() {
  // put your main code here, to run repeatedly:
   shiftOut(0xffff);
   delay(500);
   shiftOut(0x0000);
   delay(500);
}
