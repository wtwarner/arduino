
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

#include "TimerHelpers.h"
#define USE_PROGMEM
#define FIL_AC

const int PAD_SD1=3, PAD_SCLK=2, PAD_RCLK=4, PAD_SD2=5; // shift register
const int PAD_FIL0=9, PAD_FIL1=10;  // filament "A/C" waveform

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
  long a = analogRead(A7);
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

void checkVoltage()
{
  // turn off filament if voltage sags
  const int v = getVoltage();
  if (TCCR1B != 0 && v < 4500) {
    fil_disable();
  }
  else if (TCCR1B == 0 && v > 4700) {
    fil_enable();
  }
}

byte pattern = 0;
unsigned long patternMillis = 0;
const byte NUM_PATTERN = 7;

void loop() {
  checkVoltage();

  switch (pattern) {
    case 0: circle_outward(1); break;
    case 1: circle_outward(0); break;
    case 2: test1(0); break;
    case 3: rotate(0); break;
    case 4: rotate(1); break;
    case 5: spiral(0); break;
    case 6: spiral(1); break;
    default: all1(); break;
  }

  if (millis() - patternMillis > 8000l) {
    patternMillis = millis();
    pattern = (pattern + 1) % NUM_PATTERN;
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

void all1() {
  clear_vfd(1);
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
  int stepDelay = 60;
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