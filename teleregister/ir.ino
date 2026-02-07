#include "ir.h"

#define IRMP_SUPPORT_NEC_PROTOCOL 1
#define IRMP_INPUT_PIN 3
#include <irmp.hpp>

IRMP_DATA irmp_data;

void ir_setup()
{

    irmp_init();
}

bool ir_check(ir_cmd_t &cmd, bool &rep)
{
    bool rc = false;
    cmd = IR_UNKNOWN;
    rep = false;
    
    if (irmp_get_data(&irmp_data) && irmp_data.address == 0x0000)
    {
        /*
         * Serial output
         * takes 2 milliseconds at 115200
         */
        Serial.print((irmp_data.flags & IRMP_FLAG_REPETITION) ? "rep " : "    ");

        rc = true;
        cmd = irmp_data.command;
        rep = (irmp_data.flags & IRMP_FLAG_REPETITION) != 0;
        // Elegoo IR remote commands
        switch (irmp_data.command) {
            case 0x45: Serial.println("power"); break;
            case 0x46: Serial.println("vol+"); break;
            case 0x47: Serial.println("func/stop"); break;

            case 0x44: Serial.println("rewind"); break;
            case 0x40: Serial.println("play/pause"); break;
            case 0x43: Serial.println("ff"); break;

            case 0x07: Serial.println("down"); break;
            case 0x15: Serial.println("vol-"); break;
            case 0x09: Serial.println("up"); break;

            case 0x16: Serial.println("0"); break;
            case 0x19: Serial.println("EQ"); break;
            case 0x0d: Serial.println("st/rept"); break;

            case 0x0c: Serial.println("1"); break;
            case 0x18: Serial.println("2"); break;
            case 0x5e: Serial.println("3"); break;

            case 0x08: Serial.println("4"); break;
            case 0x1c: Serial.println("5"); break;
            case 0x5a: Serial.println("6"); break;

            case 0x42: Serial.println("7"); break;
            case 0x52: Serial.println("8"); break;
            case 0x4a: Serial.println("9"); break;

            default:
                Serial.println("?"); break;
                rc = false;
         }
   }
    
   return rc;
}
