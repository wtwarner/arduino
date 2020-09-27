//  SerialIn_SerialOut_HM-10_01
//
//  Uses hardware serial to talk to the host computer and AltSoftSerial for communication with the bluetooth module
//
//  What ever is entered in the serial monitor is sent to the connected device
//  Anything received from the connected device is copied to the serial monitor
//  Does not send line endings to the HM-10
//
//  Pins
//  BT VCC to Arduino 5V out. 
//  BT GND to GND

// Arduino Duo:  AltSoftSerial
//  Arduino D8 (SS RX) - BT TX no need voltage divider 
//  Arduino D9 (SS TX) - BT RX through a voltage divider (5v to 3.3v)
//
// Teensy 2.0: (HardwareSerial: Serial1)
//   D7  (RX) - from BT TX  (3.3v)
//   D8  (TX) - to BT RX through voltage divider (5v to 3.3v)

#include "common.h"

#ifdef TEENSYDUINO
#    define BTserial Serial1
#else
    // https://www.pjrc.com/teensy/td_libs_AltSoftSerial.html
#   include <AltSoftSerial.h>
    AltSoftSerial BTserial; 
#endif


struct btCmdParser {
   enum {IDLE, WAIT_EOL};
   btCmdParser(): cmd(""), state(IDLE), valid(0) {}
   
   void addChar(char c) {
    switch (state) {
      case IDLE:
        if (c == '[') {
          state = WAIT_EOL;
          cmd = "";
          valid = 0;
          //Serial.println("bt ->WAIT_EOL");
        }
        break;
      case WAIT_EOL:
        if (c == ']') {
          valid = 1;
          state = IDLE;  
          //Serial.print("bt ->IDLE"); Serial.println(cmd);
        }
        else if (c == '[') {
          valid = 0;
          cmd = "";
          // stay in WAIT_EOL
        }
        else {
          cmd += c;
          //Serial.print("bt ++"); Serial.println(cmd);
        }
    }
   }
   String cmd;
   byte state;
   byte valid;
};

btCmdParser parser;
 
void bt_setup() 
{
    BTserial.begin(9600);  

    pinMode(3, OUTPUT);
}

void bt_send(const String &cmd)
{
    BTserial.print(cmd);
}

bool bt_loop(String &cmd)
{
    // Read from the Bluetooth module and send to the Arduino Serial Monitor
    if (BTserial.available())
    {
        char c = BTserial.read();
        //Serial.write(c);
        parser.addChar(c);
    }

    if (parser.valid) {
      //Serial.print("VALID: "); Serial.println(parser.cmd);
      cmd = parser.cmd;
      parser.valid = 0;
      return true;
      
    }
    return false;
}
