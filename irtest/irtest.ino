/*
 * IR test using "IRMP" library, with Elegoo IR Remote.
 *
 *  Copyright (C) 2019-2020  Armin Joachimsmeyer
 *  armin.joachimsmeyer@gmail.com
 *
 *  This file is part of IRMP https://github.com/ukw100/IRMP.
 *
 *  IRMP is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/gpl.html>.
 *
 */

#include <Arduino.h>

#define IRMP_INPUT_PIN   21
#define IRMP_SUPPORT_NEC_PROTOCOL 1

/*
 * After setting the definitions we can include the code and compile it.
 */
#include <irmp.c.h>

IRMP_DATA irmp_data;

void setup()
{
    Serial.begin(115200);
    delay(2000);

    irmp_init();

}

void loop()
{
    int pwm = -1;
    if (irmp_get_data(&irmp_data) && irmp_data.address == 0xff00)
    {
        /*
         * Serial output
         * takes 2 milliseconds at 115200
         */
        Serial.print((irmp_data.flags & IRMP_FLAG_REPETITION) ? "rep " : "    ");

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

            case 0x16: Serial.println("0"); pwm = 0; break;
            case 0x19: Serial.println("EQ"); break;
            case 0x0d: Serial.println("st/rept"); break;

            case 0x0c: Serial.println("1"); pwm = 1; break;
            case 0x18: Serial.println("2"); pwm = 2; break;
            case 0x5e: Serial.println("3"); pwm = 3; break;

            case 0x08: Serial.println("4"); pwm = 4; break;
            case 0x1c: Serial.println("5"); pwm = 5; break;
            case 0x5a: Serial.println("6"); pwm = 6; break;

            case 0x42: Serial.println("7"); pwm = 7; break;
            case 0x52: Serial.println("8"); pwm = 8; break;
            case 0x4a: Serial.println("9"); pwm = 9; break;

            default:
                Serial.println("?"); break;
         }

    }

  delay(2);
}

