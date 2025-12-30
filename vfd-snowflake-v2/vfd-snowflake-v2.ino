
// VFD "snowflake" controller.
//
// Arduino Nano ESP32 clone
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
#include <TimeLib.h>
#include <Array.h>
#include <Preferences.h>
#include <serial-readline.h>

#include "driver/mcpwm.h"  // "arduino-esp32-master" v2.0.14   (https://github.com/espressif/arduino-esp32/)
#include "soc/mcpwm_struct.h"

#include "global.h"

const int PAD_SD1=3, PAD_SCLK=2, PAD_RCLK=4, PAD_SD2=5; // shift register
const int PAD_FIL0=9, PAD_FIL1=10;  // filament "A/C" waveform
const int PAD_FIL0_ESP32 = 18, PAD_FIL1_ESP32 = 21; // ESP native pin numbers of above, for MCPWM API
const int PAD_FIL_SR_CLK = 11, PAD_FIL_SR_MR_ = 12; // delay FIL shift register clk, reset
const int PAD_FIL_SR_CLK_ESP32 = 38;
const int PAD_MEASURE5V = A0;
const int PAD_MEASURE5V_ESP = 1;

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

g_state_t g_state = {true, 65535, false};

const vfd_cfg_t vfd_cfg[NUM_RAD][24] = { // [radius][angle]
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
bool filOn = true;

volatile uint32_t pwm_fil_period = 0;
void fil_mcpwm_isr(void *);

Preferences prefs;

//
// Serial port command line
//
void cmd_parse(String &cmd);

SerialLineReader<typeof(Serial)>  serial_reader(Serial);

void cmd_parse(String &cmd) {
  unsigned long v = 0, v2 = 0;
  if (int i = cmd.indexOf(':'); i >= 0) {
    String value_s(cmd.substring(i + 1));
    v = strtoul(value_s.c_str(), 0, 0);

    if (int i2 = cmd.indexOf(':', i + 1); i >= 0) {
      String value_s(cmd.substring(i2 + 1));
      v2 = strtoul(value_s.c_str(), 0, 0);
    }
  }

  if (cmd.startsWith("t?")) {
     Serial.println(now());
     Serial.print(hour()); Serial.print(":"); Serial.println(minute());
  }
  else if (cmd.startsWith("pwr:")) {
    g_state.enable = (v == 1);
    Serial.print("power: "); Serial.println(g_state.enable);
  }
  else if (cmd.startsWith("pat:")) {
    g_state.pattern = max(1ul, v); // disallow value 0
    Serial.print("pattern: "); Serial.println(g_state.pattern,HEX);
  }
  else if (cmd.startsWith("te:")) {
    g_state.timerEnable = (v == 1);
    if (!g_state.timerEnable) { timerOn = true; }
    Serial.print("timerEnable: "); Serial.println(g_state.timerEnable);
  }
  else if (cmd.startsWith("st:")) {
    setTime(v);
    Serial.print("curTime "); Serial.print(v); Serial.print("; "); Serial.print(hour()); Serial.print(":"); Serial.println(minute());
  }
  else if (cmd.startsWith("ont:")) {
    g_state.onTimeH = v;
    g_state.onTimeM = v2;

    Serial.print("onTime "); Serial.print(v); Serial.print(':'); Serial.println(v2);
  }
  else if (cmd.startsWith("offt:")) {
    g_state.offTimeH = v;
    g_state.offTimeM = v2;

    Serial.print("offTime "); Serial.print(v); Serial.print(':'); Serial.println(v2);
  }

  update_prefs();
}

void update_prefs()
{
  prefs.begin("snowflake", false);
  prefs.putBool("enable", g_state.enable);
  prefs.putUShort("pattern", g_state.pattern);
  prefs.putBool("timerEnable", g_state.timerEnable);
  prefs.putUChar("onTimeH", g_state.onTimeH);
  prefs.putUChar("onTimeM", g_state.onTimeM);
  prefs.putUChar("offTimeH", g_state.offTimeH);
  prefs.putUChar("offTimeM", g_state.offTimeM);
  prefs.end();
}

void setup() {
  Serial.begin(115200);
  delay(1000);  
  Serial.println("Starting");
  pinMode(PAD_SD1, OUTPUT);
  pinMode(PAD_SD2, OUTPUT);
  pinMode(PAD_SCLK, OUTPUT);
  pinMode(PAD_RCLK, OUTPUT);
  pinMode(PAD_FIL0, OUTPUT);
  pinMode(PAD_FIL1, OUTPUT);
  pinMode(PAD_FIL_SR_CLK, OUTPUT);
  pinMode(PAD_FIL_SR_MR_, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 1);
  digitalWrite(PAD_FIL_SR_MR_, 0);

  prefs.begin("snowflake", true);
  g_state.enable = prefs.getBool("enable", true);
  g_state.pattern = prefs.getUShort("pattern", 0x007b);
  g_state.timerEnable = prefs.getBool("timerEnable", false);
  g_state.onTimeH = prefs.getUChar("onTimeH", 5);
  g_state.onTimeM = prefs.getUChar("onTimeM", 30);
  g_state.offTimeH = prefs.getUChar("offTimeH", 22);
  g_state.offTimeM = prefs.getUChar("offTimeM", 30);
  prefs.end();
  
  Serial.print("eeprom pat: "); Serial.println(g_state.pattern, HEX);
  Serial.print("eeprom en: "); Serial.println(g_state.enable);
  Serial.print("eeprom timerEn: "); Serial.println(g_state.timerEnable);

  analogReadResolution(12);
  analogSetPinAttenuation(PAD_MEASURE5V, ADC_0db);

  //
  // Filament A/C clock
  //

  MCPWM0.int_ena.val = 0;
  if (ESP_OK != mcpwm_isr_register(MCPWM_UNIT_0, fil_mcpwm_isr, 0, ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_SHARED, 0)) {
    Serial.println("mcpwm_isr_register failed");
  }
  if (ESP_OK != mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, PAD_FIL0_ESP32)) {
    Serial.println("gpio_init failed");
  }
  if (ESP_OK != mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, PAD_FIL1_ESP32)) {
    Serial.println("gpio_init failed");
  }
  GPIO.func_out_sel_cfg[PAD_FIL1_ESP32].inv_sel = 1;  // invert

  mcpwm_config_t pwm_config_fil = {
    .frequency = 400,
    .cmpr_a = 50,            // Duty cycle of PWMxA
    .duty_mode = MCPWM_DUTY_MODE_0,
    .counter_mode = MCPWM_UP_COUNTER
  };
  if (ESP_OK != mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config_fil)) {
    Serial.println("mcpwm_init sr failed");
  }
  if (ESP_OK != mcpwm_set_timer_sync_output(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_SWSYNC_SOURCE_TEZ)) {
    Serial.println("mcpwm_set_timer_sync failed");
  }
  MCPWM0.timer[0].timer_cfg0.timer_period_upmethod = 1;
  MCPWM0.timer[1].timer_cfg0.timer_period_upmethod = 1;
  pwm_fil_period = MCPWM0.timer[0].timer_cfg0.timer_period;
  Serial.print("pwm0 period "); Serial.println(pwm_fil_period);
 
  //
  // Filament delay shift register clock, 16x the A/C clock
  //
  if (ESP_OK != mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM1A, PAD_FIL_SR_CLK_ESP32)) {
    Serial.println("gpio-init sr failed");
  }
  mcpwm_config_t pwm_config_fil_sr = {
    .frequency = 400*16,
    .cmpr_a = 50,
    //.cmpr_b = 50,  
    .duty_mode = MCPWM_DUTY_MODE_0,
    .counter_mode = MCPWM_UP_COUNTER
  };
  if (ESP_OK != mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_1, &pwm_config_fil_sr)) {
    Serial.println("mcpwm_init sr failed");
  }
  mcpwm_sync_config_t sync_config_sr = {
    .sync_sig = MCPWM_SELECT_TIMER0_SYNC,
    .timer_val = 0,
    .count_direction = MCPWM_TIMER_DIRECTION_UP
  };
  if (ESP_OK != mcpwm_sync_configure(MCPWM_UNIT_0, MCPWM_TIMER_1, &sync_config_sr)) {
    Serial.println("mcpwm_sync_enable failed");
  }

  fil_disable();

  clear_vfd();
  pack_vfd();
  send_vfd();

  init_wifi();
  init_time();
  init_webserver();

  // init Filament PWM interrupt
  MCPWM0.int_ena.val = BIT(6);
 
}

void IRAM_ATTR fil_mcpwm_isr(void *)
{
  // random period between [0.5, 1.5] * nominal period
  // use rand() instead of random(); the latter used true RNG and seems to deadlock when WiFi enabled?
  uint32_t period = pwm_fil_period/2 + rand() % pwm_fil_period;
  MCPWM0.timer[0].timer_cfg0.timer_period = period;
  MCPWM0.timer[1].timer_cfg0.timer_period = period/16;  
  MCPWM0.operators[0].timestamp[0].gen = period/2;
  MCPWM0.operators[1].timestamp[0].gen = period/32;
  MCPWM0.int_clr.val = BIT(6);
}

void fil_enable()
{
  digitalWrite(PAD_FIL_SR_MR_, 1);
  digitalWrite(LED_BUILTIN,1);
  Serial.println("A/C enable");
  filOn = true;
}

void fil_disable()
{
  digitalWrite(PAD_FIL_SR_MR_, 0);
  digitalWrite(LED_BUILTIN,0);
  Serial.println("A/C disable");
  filOn = false;
}

int getVoltage(void) {
  long a = analogRead(PAD_MEASURE5V);
  // pin has Vcc * 1/(1+4.7).  ADC 0..4095 is 0..1.1V.
  long mv = a * 989l/4095l; // ideally 1100/4095 for 1.1V
  long scaled_mv = mv * 5700l/1000l;
  //Serial.print("raw "); Serial.print(a); Serial.print("; raw mV "); Serial.print(mv);
  //Serial.print("; scaled mV "); Serial.println(scaled_mv);
  return scaled_mv;
}

void clear_vfd(byte value) { 
  memset(&vfd_state, value, sizeof(vfd_state));
}

void pack_vfd()
{
  memset(packed_bits, 0, sizeof(packed_bits));
  for (byte r = 0; r < NUM_RAD; r ++) { // radius
    for (byte a = 0; a < vfd_per_radius[r]; a ++) {
      vfd_cfg_t cfg;
      memcpy(&cfg, &vfd_cfg[r][a], sizeof(cfg));
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
  if (filOn && (!g_state.enable || v < 4500 || !timerOn)) {
    fil_disable();
  }
  else if (!filOn && g_state.enable && v > 4700 && timerOn) {
    fil_enable();
  }
  return filOn;
}

bool check_cmd()
{
  serial_reader.poll();
  return serial_reader.available();
}

// for calling within pattern functions; delay while checking
// for commands
#define DELAY(ms)                       \
do {                                    \
  unsigned long start = millis();       \
  while ((millis() - start) < (unsigned long)(ms)) {    \
    if (check_cmd()) {                  \
      return;                           \
    }                                   \
    delay(2);                           \
  }                                     \
} while(0)

void loop() {
  if (check_cmd()) {
    if (serial_reader.available()) {
      char s[16]; // including terminating \0
      serial_reader.read(s);
      String str(s);
      cmd_parse(str);
    }
  }
  check_webserver();

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

  unsigned long currMillis = millis();
  if (currMillis - patternMillis > 8000ul) {
    patternMillis = currMillis;
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
    DELAY(200); 
  }
}

void all_constant(byte v) {
  clear_vfd(v);
  pack_vfd();
  send_vfd();
  DELAY(100);
}

void circle_outward(byte polarity) {
  for (byte r = 0; r < NUM_RAD; r++) {
    for (byte s = 0; s < NUM_SEG; s++) {
      clear_vfd(!polarity);
      for (byte a = 0; a < vfd_per_radius[r]; a++) {
         vfd_state[r][a][s] = polarity;
      }
      pack_vfd();
      send_vfd();
      DELAY(120);
    }
  }
}

void rotate(byte dir) {
  for (byte a = dir ? 25 : 0; dir ? (a > 0) : (a < 25); a += dir ? -1 : 1) {
    clear_vfd();
    for (byte r = 0; r < NUM_RAD; r++) {
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
    DELAY(50);
  }
}

void spiral(byte polarity) {
  const int stepDelay = 40;
  clear_vfd(!polarity);

  for (byte r = 0; r < NUM_RAD; r++) {
    for (byte s = 0; s < NUM_SEG; s++) {
      for (byte a = 0; a < vfd_per_radius[r]; a++) {
        vfd_state[r][a][s] = polarity;
        pack_vfd();
        send_vfd();
        DELAY(stepDelay);
      }
    }
  }
}
