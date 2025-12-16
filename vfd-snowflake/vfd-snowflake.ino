
// VFD "snowflake" controller.
//
// Elegoo Nano 3.0 (ATMega328 old bootloader or sometimes new bootloader)
//
// This controller talks to 10 sub-controllers.
// 16 bit shift register per sub-controller (for 4 VFD tubes => 12 segments). 
// Shift register has two parallel SD pins, so 5*16 bits per SD pin.
// Currently only 9 controllers are used (0..4 on SD1, 0..3 on SD2).  So last on SD2 is dummy.
// The shift register signals include SCLK (serial bit clock), RCLK (register clock), SD1/SD2.
//
// This controller sends a square wave on 2 pins (complementary pair) to sub-cons,
// which use H-Bridges to make A/C for the filaments.
//
#include <Wire.h>
#include <TimeLib.h>
#include <EEPROM.h>
#include "TimerHelpers.h"
#include <serial-readline.h>

#define USE_PROGMEM
#define FIL_AC

const int PAD_SD1=3, PAD_SCLK=2, PAD_RCLK=4, PAD_SD2=5; // shift register
const int PAD_FIL0=9, PAD_FIL1=10;  // filament "A/C" waveform
const int PAD_MEASURE5V = A7;

void clear_vfd(byte polarity=0);
void pack_vfd();
void send_vfd();

const int NUM_SEG = 3; // VFD tube has 2 segments
const int NUM_RAD = 3; // VFD arranged into 3 radius levels
const int NUM_SUBCON = 10; // VFD subcontrollers (4 tubes per sub-con)
const int BITS_PER_SUBCON = 16;   // only 12 bits used (3 bits per tube, 4 tubes per sub-con)

enum { SEG_L, SEG_M, SEG_H};
const int tube_shift[4] = {13, 10, 5, 2};
const int vfd_per_radius[3] = { 4, 8, 24 };

struct vfd_cfg_t {
  byte seg_map[3];
  byte subcon;
  byte tube;
};

byte vfd_state[NUM_RAD][24][NUM_SEG];

struct {
  bool enable;
  uint16_t pattern;
  bool timerEnable;
  byte onTimeH, onTimeM;
  byte offTimeH, offTimeM;
} g_state = {true, 65535, false};

const 
#ifdef USE_PROGMEM
    PROGMEM 
#endif
vfd_cfg_t vfd_cfg[NUM_RAD][24] = { // [radius][angle]
  // inner ring
  { 
 
    {{ 1,0,2}, 4, 2 },  // 45 deg.
    {{ 1,0,2}, 4, 0 }, 
    {{ 1,0,2}, 4, 1 }, 
    {{ 0,1,2}, 4, 3 }, 
    {{0}}
  }, 
  // mid ring
  {
    {{ 2,0,1}, 3, 0 }, // 0 deg.
    {{ 2,0,1}, 2, 0 }, // 45 deg.
    {{ 2,0,1}, 1, 0 }, // 90 deg
    {{ 2,0,1}, 0, 0 }, // 135 deg.
    {{ 2,0,1}, 9, 0 },
    {{ 2,0,1}, 8, 0 },
    {{ 2,0,1}, 7, 0 },
    {{ 2,0,1}, 6, 0 }, // 225 deg.
  },
  // outer ring
  {
    {{ 1,0,2}, 3, 3 },
    {{ 1,0,2}, 3, 2 },
    {{ 1,0,2}, 3, 1 },

    {{ 1,0,2}, 2, 3 }, 
    {{ 1,0,2}, 2, 2 },
    {{ 1,0,2}, 2, 1 },

    {{ 1,0,2}, 1, 3 },
    {{ 1,0,2}, 1, 2 },            
    {{ 1,0,2}, 1, 1 },

    {{ 1,0,2}, 0, 3 },
    {{ 1,0,2}, 0, 2 },
    {{ 1,0,2}, 0, 1 }, 

    {{ 1,0,2}, 9, 3 },
    {{ 1,0,2}, 9, 2 },
    {{ 1,0,2}, 9, 1 }, 
    
    {{ 1,0,2}, 8, 3 },
    {{ 1,0,2}, 8, 2 },
    {{ 1,0,2}, 8, 1 },

    {{ 1,0,2}, 7, 3 },  
    {{ 1,0,2}, 7, 2 },
    {{ 1,0,2}, 7, 1 },

    {{ 1,0,2}, 6, 3 },
    {{ 1,0,2}, 6, 2 }, 
    {{ 1,0,2}, 6, 1 },


  }
};

uint8_t packed_bits[NUM_SUBCON * BITS_PER_SUBCON/8];

byte patternIndex = 0;
unsigned long patternMillis = 0;
const byte NUM_PATTERN = 7;
bool timerOn = true;

//
// Serial port command line
//
void serial_received(char *s);
void cmd_parse(String &cmd);

SerialLineReader<typeof(Serial)>  serial_reader(Serial, serial_received);

void serial_received(char *s)
{
  String str(s);
  cmd_parse(str);
}

void cmd_parse(String &cmd) {
  int v = 0, vx = 0;
  int i;
  if ((i = cmd.indexOf(':')) >= 0) {
    String value_s(cmd.substring(i + 1));
    v = atoi(value_s.c_str());
    vx = (int)strtol(value_s.c_str(), 0, 16);
  }

  if (cmd.startsWith("t?")) {
     Serial.println(now());
     Serial.print(hour()); Serial.print(":"); Serial.println(minute());
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(PAD_SD1, OUTPUT);
  pinMode(PAD_SD2, OUTPUT);
  pinMode(PAD_SCLK, OUTPUT);
  pinMode(PAD_RCLK, OUTPUT);
  pinMode(PAD_FIL0, OUTPUT);
  pinMode(PAD_FIL1, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 1);

  EEPROM.get(0, g_state);
  Wire.begin(8);                // join I2C bus with address #8
  Wire.onReceive(i2cReceiveEvent); // write
  Wire.onRequest(i2cRequestEvent); // read

  analogReference(INTERNAL);
  fil_disable();

  clear_vfd();
  pack_vfd();
  send_vfd();
}

static byte timer1_isr_state = 0;
ISR (TIMER1_OVF_vect)
{
  const uint16_t period = 1000 + random(1500);
  ICR1 = period;
  OCR1A = period/2;
  OCR1B = period/2;
}

byte i2c_cmd = 0;
bool i2c_cmd_valid = false;

enum { I2C_CMD_FIRST = 0x80, 
  I2C_CMD_POWER = 0x80,
  I2C_CMD_PATTERN, 
  I2C_CMD_CURTIME,   // Unix time_t LSB first
  I2C_CMD_ONTIME, // H then M
  I2C_CMD_OFFTIME, // H then M
  I2C_CMD_ENTIMER, // enable
  I2C_CMD_LAST};

void i2cRequestEvent()
{
  Serial.print("i2c read "); Serial.println(i2c_cmd, HEX);
   byte resp[2] = {0};
   byte respCount = 1;
   switch (i2c_cmd) {
    case I2C_CMD_POWER: resp[0] = g_state.enable; break;
    case I2C_CMD_PATTERN: resp[0] = g_state.pattern & 0xff; resp[1] = (g_state.pattern >> 8) & 0xff; respCount = 2; break;
    case I2C_CMD_ENTIMER: resp[0] = g_state.timerEnable; break;
    case I2C_CMD_ONTIME: resp[0] = g_state.onTimeH; resp[1] = g_state.onTimeM; respCount = 2; break;
    case I2C_CMD_OFFTIME: resp[0] = g_state.offTimeH; resp[1] = g_state.offTimeM; respCount = 2; break;
   }
   Wire.write(resp, respCount);
   i2c_cmd_valid = 0;
}

void i2cReceiveEvent(int howMany) {
  while (Wire.available()) { 
    if (!i2c_cmd_valid) {
      byte c = Wire.read(); 
      Serial.print("cmd: "); Serial.println(c,HEX);
      if (c >= I2C_CMD_FIRST && c <= I2C_CMD_LAST) {
        i2c_cmd = c;
        i2c_cmd_valid = true;
      }
    }
    else if (i2c_cmd_valid) {
      byte c;
      switch (i2c_cmd) {
        case I2C_CMD_POWER:
          c = Wire.read();
          g_state.enable = (c == 1);
          Serial.print("power: "); Serial.println(c,HEX);
          break;
        case I2C_CMD_PATTERN:
          c = Wire.read();
          g_state.pattern = c | (Wire.read() << 8);
          g_state.pattern = max(1, g_state.pattern); // disallow value 0
          Serial.print("pattern: "); Serial.println(g_state.pattern,HEX);
          break;
        case I2C_CMD_ENTIMER:
          c = Wire.read();
          g_state.timerEnable = c;
          if (!g_state.timerEnable) { timerOn = true; }
          Serial.print("timerEnable: "); Serial.println(c,HEX);
          break;
        case I2C_CMD_CURTIME:
        {
          time_t t = 0;
          for (byte i = 0; i < 4; i ++) {
            t = (t >> 8l) | ((time_t)Wire.read() << 24l);
          }
          Serial.print("curTime "); Serial.println(t);
          setTime(t);
          Serial.print(hour()); Serial.print(":"); Serial.println(minute());
          break;
        }
        case I2C_CMD_ONTIME:
        {
          byte h = Wire.read();
          byte m = Wire.read();
          g_state.onTimeH = h;
          g_state.onTimeM = m;

          Serial.print("onTime "); Serial.print(h); Serial.print(':'); Serial.println(m);
          break;
        }
        case I2C_CMD_OFFTIME:
        {
          byte h = Wire.read();
          byte m = Wire.read();
          g_state.offTimeH = h;
          g_state.offTimeM = m;
          
          Serial.print("offTime "); Serial.print(h); Serial.print(':'); Serial.println(m);
          break;
        }
      }
          
      i2c_cmd_valid = false;

    }
  }
}

void fil_enable()
{
#ifdef FIL_AC
  // set up Timer 1 for filament "A/C"; 2 pins opposite polarity
  TCNT1 = 0;
  ICR1 = 2500;
  TIMSK1 = bit (TOIE1);   // enable interrupt

  // Mode 1: CTC, top = OCR1A
  Timer1::setMode (10, Timer1::PRESCALE_64, Timer1::CLEAR_A_ON_COMPARE | Timer1::SET_B_ON_COMPARE);
  OCR1A = 1250;
  OCR1B = 1250;
#else
  digitalWrite(PAD_FIL0, 1);
  digitalWrite(PAD_FIL1, 0);
#endif
  digitalWrite(LED_BUILTIN,1);
  Serial.println("A/C enable");
}

void fil_disable()
{
  TCCR1A = 0;
  TCCR1B = 0;
  digitalWrite(PAD_FIL0, 0);
  digitalWrite(PAD_FIL1, 0);
  digitalWrite(LED_BUILTIN,0);
  Serial.println("A/C disable");
}

int getVoltage(void) {
  long a = analogRead(PAD_MEASURE5V);
  // pin has Vcc * 1/(1+4.7).  ADC 0..1023 is 0..1.1V.
  long mv = a * 1100l/1023l;
  long scaled_mv = mv * 5700l/1000l;
  //Serial.print("raw "); Serial.print(a); Serial.print("; raw mV "); Serial.print(mv);
  //Serial.print("; scaled mV "); Serial.println(scaled_mv);
  return scaled_mv;
}

void clear_vfd(byte value = 0) { 
  memset(&vfd_state, value, sizeof(vfd_state));
}

void pack_vfd()
{
  memset(packed_bits, 0, sizeof(packed_bits));
  for (byte r = 0; r < NUM_RAD; r ++) { // radius
    for (byte a = 0; a < vfd_per_radius[r]; a ++) {
      vfd_cfg_t cfg;
#ifdef USE_PROGMEM      
      memcpy_P(&cfg, &vfd_cfg[r][a], sizeof(cfg)); // copy from PROGMEM
#else
      memcpy(&cfg, &vfd_cfg[r][a], sizeof(cfg));
#endif            
      for (byte seg = 0; seg < NUM_SEG; seg ++) {
         int bitn = cfg.subcon * BITS_PER_SUBCON + tube_shift[cfg.tube] + cfg.seg_map[seg];
         packed_bits[bitn/8] |= vfd_state[r][a][seg] << (bitn%8);
      }
    }
  }
}

void send_vfd()
{
  const int bits_per_sd = NUM_SUBCON*BITS_PER_SUBCON/2;
  for (int bit_sd1 = 0; bit_sd1 < bits_per_sd; bit_sd1 ++) {
    int bit_sd2 = bits_per_sd + bit_sd1;
    digitalWrite(PAD_SD1, (packed_bits[bit_sd1/8] >> (bit_sd1%8)) & 1);
    digitalWrite(PAD_SD2, (packed_bits[bit_sd2/8] >> (bit_sd2%8)) & 1);
    digitalWrite(PAD_SCLK, 1);
    digitalWrite(PAD_SCLK, 0);
  }
  digitalWrite(PAD_RCLK, 1);
  digitalWrite(PAD_RCLK, 0);
}

bool checkVoltageGood()
{
  // turn off filament if voltage sags
  const int v = getVoltage();
  const bool off = (TCCR1B == 0);
  if (!off && (!g_state.enable || v < 4500 || !timerOn)) {
    fil_disable();
  }
  else if (off && g_state.enable && v > 4700 && timerOn) {
    fil_enable();
  }
  return (TCCR1B != 0);
}

void loop() {
  serial_reader.poll();

  static int prev_minute = 0;
  const int now_m = minute();
  if (now_m != prev_minute) {
    bool time_valid = now() > 1765734060l;
    const int now_minute = hour() * 60 + now_m;
    const int off_minute = g_state.offTimeH * 60 + g_state.offTimeM;
    const int on_minute = g_state.onTimeH * 60 + g_state.onTimeM;
    bool turnOn = time_valid && (now_minute == on_minute);
    bool turnOff = time_valid && (now_minute == off_minute);
    Serial.print("m: "); Serial.print(hour()); Serial.print(":"); Serial.println(now_m);
    Serial.print("now_min "); Serial.println(now_minute);

    if (g_state.timerEnable && turnOff) {
      timerOn = false;
      Serial.println("timer turn off");
    }
    else if (g_state.timerEnable && turnOn) {
      timerOn = true;
      Serial.println("timer turn on");
    }
    prev_minute = now_m;

    // write global config if changed
    EEPROM.put(0, g_state);
  }

  if (!checkVoltageGood() || !g_state.enable || g_state.pattern == 0 || (!timerOn && g_state.timerEnable)) {
    patternIndex = 255; // all zeros
  }
  else if (g_state.enable) {
    // move to next enabled pattern
    while (!(g_state.pattern & (1 << patternIndex))) {
      patternIndex = (patternIndex + 1) % NUM_PATTERN;
    }
  }

  switch (patternIndex) {
    case 0: circle_outward(1); break;
    case 1: circle_outward(0); break;
    case 2: test1(0); break;
    case 3: rotate(0); break;
    case 4: rotate(1); break;
    case 5: spiral(0); break;
    case 6: spiral(1); break;
    default: all_constant(0); break;
  }

  if (millis() - patternMillis > 8000l) {
    patternMillis = millis();
    patternIndex = (patternIndex + 1) % NUM_PATTERN;
  }
}

void test1(byte polarity) {
  for (byte s = 0; s < NUM_SEG; s++) {
    clear_vfd(!polarity);
    for (byte r = 0; r < NUM_RAD; r++) {
      for (byte a = 0; a < vfd_per_radius[r]; a++) {
        vfd_state[r][a][s] = polarity;
      }
    }
    pack_vfd();
    send_vfd();
    delay(200); 
  }
}

void all_constant(byte v) {
  clear_vfd(v);
  pack_vfd();
  send_vfd();
  delay(100);
}

void circle_outward(byte polarity) {
  for (byte r = 0; r < 3; r++) {
    for (byte s = 0; s < NUM_SEG; s++) {
      clear_vfd(!polarity);
      for (byte a = 0; a < vfd_per_radius[r]; a++) {
         vfd_state[r][a][s] = polarity;
      }
      pack_vfd();
      send_vfd();
      delay(120);
    }
  }
}

void rotate(byte dir) {
  for (byte a = dir ? 25 : 0; dir ? (a > 0) : (a < 25); a += dir ? -1 : 1) {
    clear_vfd();
    for (byte r = 0; r < 3; r++) {
      byte a_offset = (r == 0) ? 3 : 0;
      for (byte s = 0; s < NUM_SEG; s++) {
        vfd_state[r][(a + a_offset) % 24 / (24/vfd_per_radius[r])][s] = 1;
        vfd_state[r][(a + 6 + a_offset) % 24 / (24/vfd_per_radius[r])][s] = 1;
        vfd_state[r][(a + 12 + a_offset) % 24 / (24/vfd_per_radius[r])][s] = 1;
        vfd_state[r][(a + 18 + a_offset) % 24 / (24/vfd_per_radius[r])][s] = 1;
      }
    }
    pack_vfd();
    send_vfd();
    delay(50);
  }
}

void spiral(byte polarity) {
  int stepDelay = 40;
  clear_vfd(!polarity);

  for (byte r = 0; r < NUM_RAD; r++) {
    for (byte s = 0; s < NUM_SEG; s++) {
      for (byte a = 0; a < vfd_per_radius[r]; a++) {
        vfd_state[r][a][s] = polarity;
        pack_vfd();
        send_vfd();
        delay(stepDelay);
      }
    }
  }
}