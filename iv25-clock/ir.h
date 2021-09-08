#ifndef IR_H_
#define IR_H_

// for "Car MP3" remote, generic from amazon, #undef ELEGOO
#undef IR_REMOTE_ELEGOO

typedef enum {
 #ifdef IR_REMOTE_ELEGOO
    IR_POW = 0x45,
    IR_VOLUP = 0x46,
    IR_FUNCSTOP = 0x47,

    IR_REW = 0x44,
    IR_PLAY = 0x40,
    IR_FF = 0x43,

    IR_DOWN = 0x07,
    IR_VOLDOWN = 0x15,
    IR_UP = 0x09,

    IR_0 = 0x16,
    IR_EQ = 0x19,
    IR_STREPT = 0x0d,

    IR_1 = 0x0c,
    IR_2 = 0x18,
    IR_3 = 0x5e,

    IR_4 = 0x08,
    IR_5 = 0x1c,
    IR_6 = 0x5a,

    IR_7 = 0x42,
    IR_8 = 0x52,
    IR_9 = 0x4a,
 #else
    IR_CHDOWN = 0x45,
    IR_CH = 0x46,
    IR_CHUP = 0x47,

    IR_REW = 0x44,
    IR_FF = 0x40,
    IR_PLAY = 0x43,

    IR_VOLDOWN = 0x07,
    IR_VOLUP = 0x15,
    IR_EQ = 0x09,

    IR_0 = 0x16,
    IR_100 = 0x19,
    IR_200 = 0x0d,

    IR_1 = 0x0c,
    IR_2 = 0x18,
    IR_3 = 0x5e,

    IR_4 = 0x08,
    IR_5 = 0x1c,
    IR_6 = 0x5a,

    IR_7 = 0x42,
    IR_8 = 0x52,
    IR_9 = 0x4a,
 #endif

    IR_UNKNOWN = 0xff
} ir_cmd_t; 

extern void ir_setup();
extern bool ir_check(ir_cmd_t &cmd, bool &rep);

#endif
