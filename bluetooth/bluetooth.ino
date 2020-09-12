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
//  Arduino D8 (SS RX) - BT TX no need voltage divider 
//  Arduino D9 (SS TX) - BT RX through a voltage divider (5v to 3.3v)
//
 
// https://www.pjrc.com/teensy/td_libs_AltSoftSerial.html
#include <AltSoftSerial.h>
#include <EEPROM.h>

struct myEEPROM_t {
   byte pwm;
   byte led;
   };
   
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
myEEPROM_t eeprom;
AltSoftSerial BTserial; 
 
const uint8_t PROGMEM gamma8[] = {
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
  
void setup() 
{
    Serial.begin(9600);
    Serial.print("Sketch:   ");   Serial.println(__FILE__);
    Serial.print("Uploaded: ");   Serial.println(__DATE__);
    Serial.println(" ");
 
    BTserial.begin(9600);  

    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(3, OUTPUT);

    EEPROM.get(0, eeprom);
    
    Serial.print("EEPROM read pwm: "); Serial.println(eeprom.pwm);
    digitalWrite(LED_BUILTIN, eeprom.led ? HIGH : LOW);
    analogWrite(3, eeprom.pwm);
}


void loop()
{
    // Read from the Bluetooth module and send to the Arduino Serial Monitor
    if (BTserial.available())
    {
        char c = BTserial.read();
        //Serial.write(c);
        parser.addChar(c);
    }

    if (parser.valid) {
      // Serial.print("VALID: "); Serial.println(parser.cmd);
      if (parser.cmd.equalsIgnoreCase(String("on"))) {        
        eeprom.led = 1;
        digitalWrite(LED_BUILTIN, HIGH);
      }
      else if (parser.cmd.equalsIgnoreCase("off")) {
        eeprom.led = 0;
        digitalWrite(LED_BUILTIN, LOW);
      }
      else if (parser.cmd.startsWith("pwm ")) {
          byte v = atoi(parser.cmd.substring(parser.cmd.indexOf(' ')).c_str());
          //Serial.print("pwm:"); Serial.println(v);
          eeprom.pwm = pgm_read_byte(&gamma8[v]);
          analogWrite(3, eeprom.pwm);
      }
      //Serial.println("EEPROM write:");
      EEPROM.put(0, eeprom);
      parser.valid = 0;
    }
 
}
