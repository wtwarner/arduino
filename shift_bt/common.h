#ifndef COMMON_H_INC_
#define COMMON_H_INC_


extern byte gamma8(byte v);
extern byte sin8(int deg);

struct shift_state_t {
   byte pwm;
   byte mode;
   byte test_value;
};

struct neo_state_t {
   byte brightness;
};

struct myEEPROM_t {
  shift_state_t shift_state;
  neo_state_t neo_state;
};

extern void shift_setup(shift_state_t &);
extern void shift_loop(shift_state_t &);

extern void bt_setup();
extern bool bt_loop(String &cmd);
extern void bt_send(const String &cmd);

extern void neo_setup(neo_state_t &);
extern void neo_loop(neo_state_t &);

#endif
