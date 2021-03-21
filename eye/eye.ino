
// Arduino Uno

//
// https://www.codrey.com/arduino-projects/arduino-advanced-16-bit-pwm/


// 13-bit (ICR1=0x1fff) with pwm=10.
// 10k pot good results.
//

const int POT_CS = 5;
const int POT_CLK = 6;
const int POT_SDI = 7;

void setupPWM16() {
  DDRB |= _BV(PB1) | _BV(PB2); //Set pins as outputs
  TCCR1A = _BV(COM1A1) | _BV(COM1B1) //Non-Inv PWM
           | _BV(WGM11); // Mode 14: Fast PWM, TOP=ICR1
  TCCR1B = _BV(WGM13) | _BV(WGM12)
           | _BV(CS10); // Prescaler 1
  ICR1 = 0x1fff; // TOP counter value (Relieving OCR1A*)
}
//* 16-bit version of analogWrite(). Only for D9 & D10
void analogWrite16(uint8_t pin, uint16_t val)
{
  switch (pin) {
    case 9: OCR1A = val; break;
    case 10: OCR1B = val; break;
  }
}

void setup() {
  Serial.begin(9600);
  delay(1500);
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(10, OUTPUT);
  pinMode(POT_CS, OUTPUT);
  pinMode(POT_CLK, OUTPUT);
  pinMode(POT_SDI, OUTPUT);
  digitalWrite(POT_CS, 1);
  digitalWrite(POT_CLK, 0);
  digitalWrite(POT_SDI, 0);
  setupPWM16();
  analogWrite16(10, 40);
}

const uint8_t PROGMEM gamma8_table[] = {
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
  2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
  5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
  10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
  17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
  25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
  37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
  51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
  69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
  90, 92, 93, 95, 96, 98, 99, 101, 102, 104, 105, 107, 109, 110, 112, 114,
  115, 117, 119, 120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142,
  144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175,
  177, 180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213,
  215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255
};

byte gamma8(byte v) {
  return pgm_read_byte(&gamma8_table[v]);
}

void writePot(uint16_t value)
{
  uint16_t d = (value & 0x3ff) | (0 << 12) | (0 << 10);
  digitalWrite(POT_CS, 0);
  for (int i = 0; i < 16; i++) {
    digitalWrite(POT_SDI, (d >> (15 - i)) & 0x1);
    digitalWrite(POT_CLK, 1);
    digitalWrite(POT_CLK, 0);
  }
  digitalWrite(POT_CS, 1);
}
uint16_t eye_min = 17, eye_max = 128;
void iterate() {
  for (uint16_t data = eye_min; data <= eye_max; data ++) {
    byte mapped = map(data, eye_min, eye_max, 0, 255);
    byte g = gamma8(mapped);
    //analogWrite16(10, map(g, 0, 255, eye_min, eye_max));
    //writePot(map(g, 0, 255, eye_min, eye_max));
    writePot(data);
    delay(20);
  }
  for (uint16_t data = eye_max; data >= eye_min; data -- ) {
    byte mapped = map(data, eye_min, eye_max, 0, 255);
    byte g = gamma8(mapped);
    //analogWrite16(10, map(g, 0, 255, eye_min, eye_max));
    //writePot(map(g, 0, 255, eye_min, eye_max));
    writePot(data);
    delay(20);
    if (data-- == 0)
      break;
  }
}
uint16_t prev_data;
int mode = 1;
void loop() {
  //Serial.print("> ");

  if (mode == 1) {
    iterate();
    
  }
  if (Serial.available() > 0) {
    mode = 0;
  }
  while (mode == 0) {
    while (Serial.available() > 0) {

      int16_t data = Serial.parseInt();
      if (Serial.read() == '\n') {
        if (data < 0) {
          mode = 1;
          goto done;
        }
        Serial.print("["); Serial.print(data); Serial.println("]");

        //byte mapped = map(data, eye_min, eye_max, 0, 255);
        //byte g = gamma8(mapped);

        //writePot(map(g, 0, 255, eye_min, eye_max));
        writePot(data);
        prev_data = data;
        goto done;
      }
      delay(10);
    }
  }
done: ;
  }
