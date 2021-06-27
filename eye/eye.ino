
#include <math.h>

const int POT_CS = 2;
const int POT_CLK = 0;
const int POT_SDI = 7;
const int PIN_PWM = 6;

void setup() {
  Serial.begin(9600);
  delay(1500);
  Serial.println("Start");
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_PWM, OUTPUT);
  pinMode(POT_CS, OUTPUT);
  pinMode(POT_CLK, OUTPUT);
  pinMode(POT_SDI, OUTPUT);

  digitalWrite(POT_CS, 1);
  digitalWrite(POT_CLK, 0);
  digitalWrite(POT_SDI, 0);
  analogWriteFrequency(PIN_PWM, 1098.632);
  analogWriteResolution(15);
  analogWrite(PIN_PWM, 64);

  digitalWrite(LED_BUILTIN, 1);
}

static void writePot(uint16_t value)
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

uint16_t eye_min = 25, eye_max = 128;
float len_min = 3.0, len_max=17.5;
byte map_eye(byte data)
{
   float len = data/100.0 * (len_max - len_min) + len_min;
   byte g = .0095*expf(len*.267) * (eye_max - eye_min) + eye_min; 
   return constrain(g, 0, 128);
}

int led_state =0;
void iterate() {
  for (byte data = 0; data <= 100; data ++) {
   
    writePot(map_eye(data));
    delay(20);
  }
  for (byte data = 100; data >= 0; data -- ) {

    writePot(map_eye(data));
    delay(20);
    if (data-- == 0)
      break;
  }
  digitalWrite(LED_BUILTIN, led_state);
  led_state = 1 - led_state;
}

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

        byte x = map_eye(data);
        Serial.printf("[%d -> %d]\n", data, x);

        writePot(map_eye(data));

        goto done;
      }
      delay(10);
    }
  }
done: ;
  }
