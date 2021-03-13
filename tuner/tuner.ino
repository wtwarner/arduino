// Teensy 3.2

#include <Audio.h>
#include <Wire.h>
#include <AudioTuner.h>
#include "global.h"
#include "Yin16.h"

const int DAC_CLK = 10;
const int DAC_DATA = 11;
const int DAC_LOAD = 12;
const int AUDIO_IN_PIN = A1;

const int VFD_PIN_CLK = 10; // shared with DAC
const int VFD_PIN_STROBE = 8;
const int VFD_PIN_DATA = 11; // shared with DAC
const int VFD_PIN_BLANK = 9;

const int DAC_IN9_LEFT = 1;
const int DAC_IN9_RIGHT = 0;
const int MAX_VAL = 250;
const int MIN_VAL = 20;

#define USE_NOTEFREQ 1
#define USE_YIN 1

#if 0
#define MON if (1)
#define PLOT if (0)
#else
#define MON if (0)
#define PLOT if (1)
#endif

extern float find_note(float freq, const char * &name, int &octave);

const int DECIMATE_FACTOR = 4;
const int NUM_BUFFERS = 18;

// GUItool: begin automatically generated code
AudioInputAnalog         adc1(AUDIO_IN_PIN);           //xy=153,113
AudioFilterBiquad        biquad1;
#if USE_NOTEFREQ
AudioTuner               notefreq1;
AudioConnection          patchCord4(adc1, notefreq1);
#endif
#if USE_YIN
AudioConnection          patchCord3(adc1, biquad1);
AudioRecordQueue         queue1;         //xy=332,215
AudioConnection          patchCord2(biquad1, queue1);
#endif
AudioAnalyzeRMS          rms1;
AudioConnection          patchCord5(adc1, rms1);

// GUItool: end automatically generated code
#if USE_YIN
Yin16 yin16;
#endif

// For AudioTuner, use decimation_Factor 4
int16_t fir_11029_HZ_coefficients[22] =
{
  0, 3, 6, -11, -71, 21,
  352, -15, -1202, -6, 5011, 8209,
  5011, -6, -1202, -15, 352, 21,
  -71, -11, 6, 3
};


const int NUM_HIST = 5;
float freq_hist[NUM_HIST];
float weight_hist[NUM_HIST] = {0};

float update_freq_hist(float f, float w)
{
  float freq_sum = f * w;
  float weight_sum = w;
  for (int i = 1; i < NUM_HIST; i++) {
    float age_w = (float)i / (NUM_HIST - 1);
    freq_hist[i - 1] = freq_hist[i];
    freq_sum += freq_hist[i] * weight_hist[i] * age_w;
    weight_hist[i - 1] = weight_hist[i];
    weight_sum += weight_hist[i] * age_w;
  }
  freq_hist[NUM_HIST - 1] = f;
  weight_hist[NUM_HIST - 1] = w;
  return freq_sum / weight_sum;
}


void setup() {

  AudioMemory(30);

  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  delay(1000);
  MON Serial.println("start..");

  digitalWriteFast(LED_BUILTIN, 1);
  pinMode(DAC_CLK, OUTPUT);
  pinMode(DAC_DATA, OUTPUT);
  pinMode(DAC_LOAD, OUTPUT);
  digitalWrite(DAC_CLK, LOW);
  digitalWrite(DAC_DATA, HIGH);
  digitalWrite(DAC_LOAD, HIGH);
  vfd_setup();

  biquad1.setLowpass(0, 20000 / DECIMATE_FACTOR, .7);

#if USE_NOTEFREQ
  notefreq1.begin(.15, fir_11029_HZ_coefficients, sizeof(fir_11029_HZ_coefficients), DECIMATE_FACTOR);
#endif
#if USE_YIN
  Yin_init(&yin16, 128 * NUM_BUFFERS / DECIMATE_FACTOR, 0.05, AUDIO_SAMPLE_RATE_EXACT / DECIMATE_FACTOR);
  queue1.begin();
#endif
}



#if USE_NOTEFREQ
bool do_nf(float &freq, float &filt_freq)
{
  if (notefreq1.available()) {

    freq = notefreq1.read();
    const float prob = notefreq1.probability();
    //    MON Serial.printf("NF %03.2f, %2.1f prob\n",
    //       freq, prob);

    if (prob > 0.98) {
      filt_freq = update_freq_hist(freq, prob);
      return true;
    }

  }

  return false;
}
#endif



void dac_write(byte which, byte value)
{

  byte range = 0;
  uint16_t v16 = (which << 9) | (range << 8) | value;

  for (byte b = 0; b < 16; b ++) {
    digitalWrite(DAC_CLK, HIGH);
    digitalWrite(DAC_DATA, (v16 >> (15 - b)) & 1);
    //delayMicroseconds(1); // setup data to clk_
    digitalWrite(DAC_CLK, LOW);
  }
  //delayMicroseconds(1); // setup clk_ to load
  digitalWrite(DAC_LOAD, LOW);
  delayMicroseconds(1); // 250 ns min pulse
  digitalWrite(DAC_LOAD, HIGH);

}

int v = 100;
int phase = 0;

void animate() {
  //delay(3);

  if (phase && v-- < MIN_VAL) {
    phase = 1 - phase;
    v = 0;

    dac_write(DAC_IN9_LEFT, v);
    //delay(10);
    v = MIN_VAL;
  }
  else if (!phase && v++ >= MAX_VAL) {
    phase = 1 - phase;
    v = 0;

    dac_write(DAC_IN9_LEFT, v);
    //delay(10);
    v = MAX_VAL;
  }

  dac_write(DAC_IN9_LEFT, v);
}

// mVpp to rms (gain 2x)
//   10: .000
//   20: .008
//   50: .02
//  100: .041
//  250: .101
//  500: .202
// 1000: .405

long last_valid = 0;
void loop()
{

  bool nf_valid = false;
  float nf_freq, nf_filt_freq;
  bool yin_valid = false;
  float yin_freq, yin_filt_freq;

  const float rms = rms1.available()  ? rms1.read() : 0.0;

#if USE_NOTEFREQ
  nf_valid = rms >= 0.06 && do_nf(nf_freq, nf_filt_freq);
#endif

  // Yin algorithm is better, but slower.  NF algorithm is faster.
  // Use Yin when last result is old, or low signal (rms).
#if USE_YIN
  yin_valid = ((!nf_valid && ((millis() - last_valid) > 10)) || rms < 0.06) && do_yin(yin_freq, yin_filt_freq);
#endif
  if (nf_valid || yin_valid) {

    float filt_freq = yin_valid ? yin_filt_freq : nf_filt_freq;
    
    const char *note_name;
    int octave;
    const float nearest = find_note(filt_freq, note_name, octave);
    // cent = 1200 * ln(f1/f2) / ln(2) == 1200 * log2(f1/f2)
    float cent = 1200.0 * log2(filt_freq / nearest);
    //MON Serial.printf("Note: %s\n", note_name);
    vfd_write_note(note_name);
    dac_write(DAC_IN9_LEFT, map(min((int)(cent + 0.5), 0), -50, 0, MAX_VAL, MIN_VAL));
    dac_write(DAC_IN9_RIGHT, map(max((int)(cent + 0.5), 0), 0, 50, MIN_VAL, MAX_VAL));
    
    static float last_nf; 
    static float last_yin; 
    PLOT Serial.printf("%f, %f %f\n",
      filt_freq, yin_valid ? yin_freq : last_yin, nf_valid ? nf_freq : last_nf);
      if (nf_valid) last_nf = nf_freq;
      if (yin_valid) last_yin = yin_freq;
    //      MON Serial.printf("yin %3.1f, nf %3.1f, rms %.3f\n", yin_valid ? yin_freq : 0.0, nf_valid ? nf_freq : 0.0, rms);
    digitalWriteFast(LED_BUILTIN, !digitalReadFast(LED_BUILTIN));

    if (yin_valid) last_valid = millis();
  }


}

#if USE_YIN
static int16_t mem[128 * NUM_BUFFERS / DECIMATE_FACTOR];
static int buf_idx = 0;

bool do_yin(float &freq, float &filt_freq) {

  // discard oldest unneeded buffers
  while (queue1.available() > NUM_BUFFERS) {
    queue1.readBuffer();
    queue1.freeBuffer();
  }
  // retrieve and store buffer
  if (queue1.available()) {
    int16_t *bfr = queue1.readBuffer();

    for (int s = 0; s < 128;  s += DECIMATE_FACTOR, bfr += DECIMATE_FACTOR) {
      //Serial.printf("%d\n", *bfr);
      mem[(buf_idx * 128 + s) / DECIMATE_FACTOR] = *bfr;

    }
    queue1.freeBuffer();
    ++ buf_idx;
  }
  // when accumulate enough, continue processing below
  if (buf_idx == NUM_BUFFERS) {
    buf_idx = 0;
    // continue below
  }
  else {
    return false;
  }


  freq = Yin_getPitch(&yin16, mem);
  float prob = Yin_getProbability(&yin16);

  if (prob >= .95) {
    filt_freq = update_freq_hist(freq, prob * 3.0);
    return true;
  }

  return false;
}
#endif
