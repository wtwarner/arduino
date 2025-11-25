
// VFD "snowflake" controller.
//
// Elegoo Nano 3.0 (ATMega328 old bootloader)
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

const int PAD_SD1=3, PAD_SCLK=2, PAD_RCLK=4, PAD_SD2=5; // shift register
const int PAD_FIL0=9, PAD_FIL1=10;  // filament "A/C" waveform

void clear_vfd();
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
  const byte seg_map[3];
  const byte subcon;
  const byte tube;
};

byte vfd_state[NUM_RAD][24][NUM_SEG];

PROGMEM const vfd_cfg_t vfd_cfg[NUM_RAD][24] = { // [radius][angle]
  // inner ring
  { 
    {{ 2,0,1}, 0, 3 }, // 45 deg.
    {{ 2,0,1}, 0, 2 }, // 135 deg.
    {{ 2,0,1}, 0, 1 }, // 225 deg.
    {{ 2,1,0}, 0, 0 }, // 315 deg.  BUG in sub-con proto board
    {{0}}
  }, 
  // mid ring
  {
    {{ 2,0,1}, 1, 3 }, // 0 deg.
    {{ 2,0,1}, 2, 3 }, // 45 deg.
    {{ 2,0,1}, 3, 3 }, // 90 deg
    {{ 2,0,1}, 4, 3 }, // 135 deg.
    {{ 2,0,1}, 5, 3 },
    {{ 2,0,1}, 6, 3 },
    {{ 2,0,1}, 7, 3 },
    {{ 2,0,1}, 8, 3 }, // 225 deg.
  },
  // outer ring
  {
    {{ 2,0,1}, 1, 0 },
    {{ 2,0,1}, 1, 1 },
    {{ 2,0,1}, 1, 2 },

    {{ 2,0,1}, 2, 0 }, 
    {{ 2,0,1}, 2, 1 },
    {{ 2,0,1}, 2, 2 },

    {{ 2,0,1}, 3, 0 },
    {{ 2,0,1}, 3, 1 },            
    {{ 2,0,1}, 3, 2 },

    {{ 2,0,1}, 4, 0 },
    {{ 2,0,1}, 4, 1 },
    {{ 2,0,1}, 4, 2 }, 

    {{ 2,0,1}, 5, 0 },
    {{ 2,0,1}, 5, 1 },
    {{ 2,0,1}, 5, 2 },

    {{ 2,0,1}, 6, 0 },  
    {{ 2,0,1}, 6, 1 },
    {{ 2,0,1}, 6, 2 },

    {{ 2,0,1}, 7, 0 },
    {{ 2,0,1}, 7, 1 }, 
    {{ 2,0,1}, 7, 2 },

    {{ 2,0,1}, 8, 0 },
    {{ 2,0,1}, 8, 1 },
    {{ 2,0,1}, 8, 2 }, 
  }
};

uint8_t packed_bits[NUM_SUBCON * BITS_PER_SUBCON];

void setup() {
  Serial.begin(9600);
  pinMode(PAD_SD1, OUTPUT);
  pinMode(PAD_SD2, OUTPUT);
  pinMode(PAD_SCLK, OUTPUT);
  pinMode(PAD_RCLK, OUTPUT);
  pinMode(PAD_FIL0, OUTPUT);
  pinMode(PAD_FIL1, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  // set up Timer 0 for filament "A/C"; 2 pins opposite polarity
  TCNT0 = 0;
  OCR1A = 128;
  OCR1B = 128;
  // Mode 1: CTC, top = OCR1A
  Timer1::setMode (1, Timer1::PRESCALE_64, Timer1::CLEAR_A_ON_COMPARE | Timer1::SET_B_ON_COMPARE);
 
  clear_vfd();
  pack_vfd();
  send_vfd();
}

void clear_vfd() {
  for (byte r = 0; r < NUM_RAD; r++) {  // radius
    for (byte a = 0; a < 24; a++) {
      for (byte s = 0; s < NUM_SEG; s ++) {
        vfd_state[r][a][s] = 0;
      }
    }
  }
}

void pack_vfd()
{
  memset(packed_bits, 0, sizeof(packed_bits));
  for (byte r = 0; r < NUM_RAD; r ++) { // radius
    for (byte a = 0; a < vfd_per_radius[r]; a ++) {
      for (byte seg = 0; seg < NUM_SEG; seg ++) {
         int bitn = vfd_cfg[r][a].subcon * BITS_PER_SUBCON + tube_shift[vfd_cfg[r][a].tube] + vfd_cfg[r][a].seg_map[seg];
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

void loop() {
 test1();
}


void test1() {
  for (byte s = 0; s < NUM_SEG; s++) {
    clear_vfd();
    for (byte r = 0; r < NUM_RAD; r++) {
      for (byte a = 0; a < vfd_per_radius[r]; a++) {
        vfd_state[r][a][s] = 1;
      }
    }
    pack_vfd();
    send_vfd();
    delay(200);
  }
}