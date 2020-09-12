/*
  control 2 shift registers
  */
  
const int latchPin = 8;
const int clockPin = 12;
const int dataPin = 9; // LED is 11
const int dimPin = 10;

const int NUM_LEDS = 16;

struct led_state_t {
  led_state_t(): 
    target(0x81), // 50%
    prev_time(0),
    value(0x80), // 50%
    state(0) 
    {}
      
  byte get_state() const { return state; }
  void advance(unsigned long time) {
        
      if (time > prev_time) {
        unsigned long delta = time - prev_time;
        if (value < 100*256)
          value += delta * (state * 256);
        if (value > -100*256)
          value -= delta * target;
      }
      state = value < target;
      prev_time = time; 
  }

  int target;  // .8 fraction 
  unsigned long prev_time; 
  int value; // .8 fraction
  byte state;
};

led_state_t leds[NUM_LEDS];
elapsedMillis builtin_led_timer;
byte builtin_led_state = 0;

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(dimPin, OUTPUT);

  Serial.begin(9600);
 }

void loop() {
  analogWrite(dimPin, 255-128);
  byte shiftData [NUM_LEDS/8] = {0};
  unsigned long time = millis();
  for (unsigned int i = 0; i < NUM_LEDS; i ++) {
    leds[i].advance(time);
    shiftData[i/8] |= leds[i].get_state() << (i%8);
  }
  digitalWrite(latchPin, LOW);
  for (unsigned int i = 0; i < 16/8; i ++) {
    shiftOut(dataPin, clockPin, MSBFIRST, shiftData[i]);
  }
  digitalWrite(latchPin, HIGH);
  // Serial.println(leds[0].value);

  //delay(1);
  if (builtin_led_timer > 250) {
      digitalWrite(LED_BUILTIN, builtin_led_state);
      builtin_led_state = 1 - builtin_led_state;
      builtin_led_timer = 0;
  }

}
