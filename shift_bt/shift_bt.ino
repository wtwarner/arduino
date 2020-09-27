/*
  control 2 shift registers
  */


#include "common.h"

#include <EEPROM.h>


   
elapsedMillis builtin_led_timer;
byte builtin_led_state = 0;


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
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };

const uint8_t PROGMEM sin8_table[360] = 
{
128,130,132,134,136,139,141,143,
145,147,150,152,154,156,158,160,
163,165,167,169,171,173,175,177,
179,181,183,185,187,189,191,193,
195,197,199,201,202,204,206,208,
209,211,213,214,216,218,219,221,
222,224,225,227,228,229,231,232,
233,234,236,237,238,239,240,241,
242,243,244,245,246,247,247,248,
249,249,250,251,251,252,252,253,
253,253,254,254,254,255,255,255,
255,255,255,255,255,255,255,255,
254,254,254,253,253,253,252,252,
251,251,250,249,249,248,247,247,
246,245,244,243,242,241,240,239,
238,237,236,234,233,232,231,229,
228,227,225,224,222,221,219,218,
216,214,213,211,209,208,206,204,
202,201,199,197,195,193,191,189,
187,185,183,181,179,177,175,173,
171,169,167,165,163,160,158,156,
154,152,150,147,145,143,141,139,
136,134,132,130,128,125,123,121,
119,116,114,112,110,108,105,103,
101,99,97,95,92,90,88,86,
84,82,80,78,76,74,72,70,
68,66,64,62,60,58,56,54,
53,51,49,47,46,44,42,41,
39,37,36,34,33,31,30,28,
27,26,24,23,22,21,19,18,
17,16,15,14,13,12,11,10,
9,8,8,7,6,6,5,4,
4,3,3,2,2,2,1,1,
1,0,0,0,0,0,0,0,
0,0,0,0,1,1,1,2,
2,2,3,3,4,4,5,6,
6,7,8,8,9,10,11,12,
13,14,15,16,17,18,19,21,
22,23,24,26,27,28,30,31,
33,34,36,37,39,41,42,44,
46,47,49,51,53,54,56,58,
60,62,64,66,68,70,72,74,
76,78,80,82,84,86,88,90,
92,95,97,99,101,103,105,108,
110,112,114,116,119,121,123,125,
};

myEEPROM_t eeprom;

byte gamma8(byte v) {
  return pgm_read_byte(&gamma8_table[v]);
}
byte sin8(int deg) {
  return pgm_read_byte(&sin8_table[deg]);
}

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(9600);

  EEPROM.get(0, eeprom);
 
  bt_setup();
  shift_setup(eeprom.shift_state);
  neo_setup(eeprom.neo_state);
}

void loop() {

  if (builtin_led_timer > 250) {
      digitalWrite(LED_BUILTIN, builtin_led_state);
      builtin_led_state = 1 - builtin_led_state;
      builtin_led_timer = 0;
      String s = "led:" + String(builtin_led_state) + ";";
      bt_send(s); 
      s = "ledpwm:" + String(eeprom.shift_state.pwm) + ";";
      bt_send(s);
      s = "neopwm:" + String(eeprom.neo_state.brightness) + ";";
      bt_send(s);
      s = "stest:" + String(eeprom.shift_state.test_value) + ";";
      bt_send(s);
      s = "modeled0:" + String(eeprom.shift_state.mode == 0) + ";";
      bt_send(s);
      s = "modeled1:" + String(eeprom.shift_state.mode == 1) + ";";
      bt_send(s);
      s = "modeled2:" + String(eeprom.shift_state.mode == 2) + ";";
      bt_send(s);     
  }

  String bt_cmd;
  if (bt_loop(bt_cmd)) {
    if (bt_cmd.startsWith("ledbright ")) {
          eeprom.shift_state.pwm = atoi(bt_cmd.substring(bt_cmd.indexOf(' ')).c_str());
          //Serial.print("pwm:"); Serial.println(v);
    }
    else if (bt_cmd.startsWith("ledmode ")) {
          eeprom.shift_state.mode = atoi(bt_cmd.substring(bt_cmd.indexOf(' ')).c_str());
          //Serial.print("pwm:"); Serial.println(v);
    }
    else if (bt_cmd.startsWith("stest ")) {
      int pad_direction = atoi(bt_cmd.substring(bt_cmd.indexOf(' ')).c_str()); 
      switch (pad_direction) {
          case 1: eeprom.shift_state.test_value = max(eeprom.shift_state.test_value - 1, 0); break;
          case 2: eeprom.shift_state.test_value ++; break;
          case 3: eeprom.shift_state.test_value = max(eeprom.shift_state.test_value - 10, 0); break;
          case 4: eeprom.shift_state.test_value += 10; break;
      }
      Serial.print("stest "); Serial.println(eeprom.shift_state.test_value);
    }
    if (bt_cmd.startsWith("neobright ")) {
          eeprom.neo_state.brightness = atoi(bt_cmd.substring(bt_cmd.indexOf(' ')).c_str());
          //Serial.print("pwm:"); Serial.println(v);
    }

    EEPROM.put(0, eeprom);
  }

  shift_loop(eeprom.shift_state);
  neo_loop(eeprom.neo_state);
}
