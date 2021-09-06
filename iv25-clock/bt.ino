//  SerialIn_SerialOut_HM-10_01
//
//  Uses hardware serial to talk to the host computer and AltSoftSerial for communication with the bluetooth module
//


#define BTserial Serial1

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
        Serial.write(c);
        parser.addChar(c);
    }

    if (parser.valid) {
      Serial.print("VALID: "); Serial.println(parser.cmd);
      cmd = parser.cmd;
      parser.valid = 0;
      return true;

    }
    return false;
}
