#ifndef COMMON_H_INC_
#define COMMON_H_INC_

extern void bt_setup();
extern bool bt_loop(String &cmd);

extern void neo_setup();
extern void neo_loop();

extern const uint8_t PROGMEM gamma8_table[];
extern byte gamma8(byte v);

struct shift_state_t {
   byte pwm;
};

struct neo_state_t {
   byte brightness;
};

struct myEEPROM_t {
  shift_state_t shift_state;
  neo_state_t neo_state;
};
#endif
