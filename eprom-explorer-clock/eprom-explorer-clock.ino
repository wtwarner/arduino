//
// Arduino Nano R4
//

//
// EEPROM programming of type 2716
//
#define SHIFT_DATA 2
#define SHIFT_CLK_H 4
#define SHIFT_CLK_L 3
#define EEPROM_D0 5
#define EEPROM_D7 12
#define EEPROM_CE_ 13
#define EEPROM_WE_ PIN_A0
#define EEPROM_OE_ PIN_A1

extern const PROGMEM unsigned char exp_10_bin[];


/*
 * Output the address bits and outputEnable signal using shift registers.
 */
void setAddress(int address) {
  shiftOut(SHIFT_DATA, SHIFT_CLK_H, MSBFIRST, address >> 8);
  shiftOut(SHIFT_DATA, SHIFT_CLK_L, MSBFIRST, address);

 
}


/*
 * Read a byte from the EEPROM at the specified address.
 */
byte readEEPROM(int address) {
  for (int pin = EEPROM_D0; pin <= EEPROM_D7; pin += 1) {
    pinMode(pin, INPUT_PULLUP);
  }

  setAddress(address);
  digitalWrite(EEPROM_CE_, LOW);
  digitalWrite(EEPROM_OE_, LOW);
 
  byte data = 0;
  for (int pin = EEPROM_D7; pin >= EEPROM_D0; pin -= 1) {
    data = (data << 1) + digitalRead(pin);
  }
  digitalWrite(EEPROM_CE_, HIGH);
  digitalWrite(EEPROM_OE_, HIGH);
  return data;
}



/*
 * Write a byte to the EEPROM at the specified address.
 */
void writeEEPROM(int address, byte data) {
  
  setAddress(address);
  for (int pin = EEPROM_D0; pin <= EEPROM_D7; pin += 1) {
    pinMode(pin, OUTPUT);
  }
  digitalWrite(EEPROM_CE_, LOW);

  for (int pin = EEPROM_D0; pin <= EEPROM_D7; pin += 1) {
    digitalWrite(pin, data & 1);
    data = data >> 1;
  }
  digitalWrite(EEPROM_WE_, LOW);
  delay(10);
  digitalWrite(EEPROM_WE_, HIGH);
  delay(1);
  digitalWrite(EEPROM_CE_, HIGH);

}


/*
 * Read the contents of the EEPROM and print them to the serial monitor.
 */
void printContents(int len=2048) {
  for (int base = 0; base < len; base += 16) {
    byte data[16];
    for (int offset = 0; offset <= 15; offset += 1) {
      data[offset] = readEEPROM(base + offset);
    }

    char buf[80];
    sprintf(buf, "%03x:  %02x %02x %02x %02x %02x %02x %02x %02x   %02x %02x %02x %02x %02x %02x %02x %02x",
            base, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
            data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);

    Serial.println(buf);
  }
}

unsigned char PROGMEM test_bin[] = {
  0x1, 0x2, 0x3, 0x4,  
};
unsigned int test_bin_len = 4;

void setup() {
  // put your setup code here, to run once:
  pinMode(EEPROM_WE_, OUTPUT);
  digitalWrite(EEPROM_WE_, HIGH);
  pinMode(SHIFT_DATA, OUTPUT);
  pinMode(SHIFT_CLK_H, OUTPUT);
  pinMode(SHIFT_CLK_L, OUTPUT);
  pinMode(EEPROM_CE_, OUTPUT);
  digitalWrite(EEPROM_CE_, HIGH);
  pinMode(EEPROM_OE_, OUTPUT);
  digitalWrite(EEPROM_OE_, HIGH);
  for (int pin = EEPROM_D0; pin <= EEPROM_D7; pin += 1) {
    pinMode(pin, INPUT_PULLUP);
  }

  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
}

void erase_rom() {
  // Erase entire EEPROM
  Serial.print("Erasing EEPROM");
  for (int address = 0; address <= 2047; address += 1) {
    writeEEPROM(address, 0xff);

    if (address % 64 == 0) {
      Serial.print(".");
    }
  }
  Serial.println(" done");
}

const PROGMEM unsigned char * get_rom_data(int rom, int &rom_len)
{
    switch (rom) {
      case 0: 
        rom_len = test_bin_len;
        return test_bin;
      case 1:
        rom_len = 2048;
        return exp_10_bin;
      default:
          Serial.print("bad ROM index");
          return 0;
    }
}

void prog_rom(int rom)
{
  int rom_len;
  const PROGMEM unsigned char *rom_data = get_rom_data(rom, rom_len);
  if (rom_data == 0)
      return;
  // Program data bytes
  Serial.print("Programming EEPROM");
  for (int address = 0; address < rom_len; address += 1) {
      writeEEPROM(address, pgm_read_byte(rom_data + address));
      
      if (address % 64 == 0) {
          Serial.print(".");
      }
  }
  Serial.println(" done");
}

void comp_rom(int rom)
{
    int rom_len;
    const PROGMEM unsigned char *rom_data = get_rom_data(rom, rom_len);
    if (rom_data == 0)
        return;
    int mismatches = 0;
    for (int address = 0; address < rom_len; address += 1) {
        uint8_t exp = pgm_read_byte(rom_data + address);
        uint8_t got = readEEPROM(address);
        if (exp != got) {
            Serial.print("MISMATCH @0x"); Serial.print(address, HEX);
            Serial.print(" exp "); Serial.print(exp, HEX);
            Serial.print(" got "); Serial.println(got, HEX);
            mismatches ++;
        }
        if (mismatches > 10) {
            Serial.println("...");
            break;
        }
    }
    if (mismatches == 0) {
        Serial.println("PASS");
    }
    else {
        Serial.println("FAIL");
    }
}

void read_rom()
{

  // Read and print out the contents of the EERPROM
  Serial.println("Reading EEPROM");
  printContents();
}


void loop() {

    Serial.println("Command:");
    Serial.println("e - erase");
    Serial.println("p N - program rom N");
    Serial.println("c N - compare with rom N");
    Serial.println("r - read");
    Serial.print("> ");
    
    while (Serial.available() == 0)
        ;
    
    char cmd = Serial.read();
    switch (cmd) {
        case 'e':
            erase_rom();
            break;
        case 'p': {
            int i = Serial.parseInt();
            Serial.print("Programming ROM ");
            Serial.println(i);
            prog_rom(i);
            break;
        }
        case 'c': {
            int i = Serial.parseInt();
            Serial.print("Compare ROM ");
            Serial.println(i);
            comp_rom(i);
            break;
        }
        case 'r':
            read_rom();
            break;
    }
    Serial.readStringUntil('\n');
}
