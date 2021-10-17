//
// metronome
// for Teensy 4.0 with Audio Adapter and Adafruit TPA2016 board.
// 

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Bounce.h>
#include <ADC.h>
#include <ADC_util.h>
#include <FIR.h>

#include "Adafruit_TPA2016.h"

// WAV files converted to code by wav2sketch
#include "AudioSampleSnare.h"        // http://www.freesound.org/people/KEVOY/sounds/82583/
#include "AudioSampleTomtom.h"       // http://www.freesound.org/people/zgump/sounds/86334/
#include "AudioSampleHihat.h"        // http://www.freesound.org/people/mhc/sounds/102790/
#include "AudioSampleKick.h"         // http://www.freesound.org/people/DWSD/sounds/171104/

// Create the Audio components.  These should be created in the
// order data flows, inputs/sources -> processing -> outputs
//
AudioPlayMemory    playMem[2];  
AudioPlaySdWav     playWav[2]; 

AudioMixer4        mix1;    //  4-channel mixer
AudioAmplifier     amp1;
AudioOutputI2S     lineout;

// Create Audio connections between the components
//
AudioConnection c1(playMem[0], 0, mix1, 0);
AudioConnection c2(playMem[1], 0, mix1, 1);
AudioConnection c3(playWav[0], 0, mix1, 2);
AudioConnection c4(playWav[1], 0, mix1, 3);

AudioConnection c5(mix1, 0, amp1, 0);

AudioConnection c8(amp1, 0, lineout, 0);
AudioConnection c9(amp1, 0, lineout, 1);

// Create an object to control the audio shield.
// 
AudioControlSGTL5000 audioShield;

Adafruit_TPA2016 audioamp = Adafruit_TPA2016();

const int Pin_bpm = 14; // ADC0 - bpm
const int Pin_vol = 15; // ADC1 - volume
const int Pin_but = 6;
const int Pin_pwm = 9;

ADC *adc = new ADC(); // adc object;

/*

FIR filter designed with
http://t-filter.appspot.com

sampling frequency: 250 Hz

* 0 Hz - 10 Hz
  gain = 1
  desired ripple = 2 dB
  actual ripple = 1.514479888060941 dB

* 20 Hz - 125 Hz
  gain = 0
  desired attenuation = -40 dB
  actual attenuation = -40.14248267792653 dB

*/

#define FILTER_TAP_NUM 35

static double filter_taps[FILTER_TAP_NUM] = {
  -0.008432094625382603,
  -0.008630111443573948,
  -0.01177925761283797,
  -0.014453857041655421,
  -0.016002557060047697,
  -0.015738781213642406,
  -0.013005301456817076,
  -0.007342737542962018,
  0.0014881882681261293,
  0.013387274984891713,
  0.02790987467333995,
  0.04425987160012943,
  0.06133877871258253,
  0.07786745651609385,
  0.09250361539555096,
  0.10400265312748039,
  0.11134871825495105,
  0.11387368999722183,
  0.11134871825495105,
  0.10400265312748039,
  0.09250361539555096,
  0.07786745651609385,
  0.06133877871258253,
  0.04425987160012943,
  0.02790987467333995,
  0.013387274984891713,
  0.0014881882681261293,
  -0.007342737542962018,
  -0.013005301456817076,
  -0.015738781213642406,
  -0.016002557060047697,
  -0.014453857041655421,
  -0.01177925761283797,
  -0.008630111443573948,
  -0.008432094625382603
};

FIR<double, FILTER_TAP_NUM> fir;

#define SDCARD_CS_PIN    10

static const int PIN_CLK = 2;
static const int PIN_SDI = 0;
static const int PIN_STROBE = 1;

Bounce button = Bounce(Pin_but, 5);

static const float MAX_BPM = 400.0;
static const float MIN_BPM = 40.0;
int beat = 0;
float beat_timer_ms = 500.0; // 120 bpm
int meter_denom = 4;
elapsedMillis beat_timer;

elapsedMillis pot_timer;

void setup() {
  pinMode(Pin_bpm, INPUT);
  pinMode(Pin_vol, INPUT);
  pinMode(Pin_but, INPUT);
  pinMode(Pin_pwm, OUTPUT);
  analogWrite(Pin_pwm, 192);


  pinMode(PIN_CLK, OUTPUT);
  pinMode(PIN_SDI, OUTPUT);
  pinMode(PIN_STROBE, OUTPUT);
  digitalWrite(PIN_CLK, 0);
  digitalWrite(PIN_SDI, 0);
  digitalWrite(PIN_STROBE, 0);

  adc->adc0->setAveraging(32);
  adc->adc0->setResolution(10);
  adc->adc0->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_LOW_SPEED);
  adc->adc0->setSamplingSpeed(ADC_SAMPLING_SPEED::LOW_SPEED);

  adc->adc1->setAveraging(32);
  adc->adc1->setResolution(12);
  adc->adc1->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_LOW_SPEED);
  adc->adc1->setSamplingSpeed(ADC_SAMPLING_SPEED::LOW_SPEED);
  
  audioamp.begin();
  // "Rock" settings: ratio 2:1, attack 3.84 ms/6db, release 4110 ms/6db, hold --, fixed gain 6, limiter 8
  audioamp.enableChannel(false, false);
  audioamp.setAGCCompression(TPA2016_AGC_OFF);
#if 0
  audioamp.setAttackControl(int(3.84/1.28 + 0.5));
  audioamp.setReleaseControl(int(0/1644 + 0.5));
  audioamp.setHoldControl(int(0/*.2740*/ / .0137 + 0.5));
#endif
  audioamp.setGain(28);

  AudioMemory(10);

  audioShield.enable();
  audioShield.volume(.5);

  mix1.gain(0, 1.0);
  mix1.gain(1, 1.0);
  mix1.gain(2, 1.0);

  amp1.gain(0.15);

  if (!SD.begin(SDCARD_CS_PIN)) {
    Serial.println("Unable to access SD card");
  }

  fir.setFilterCoeffs(filter_taps);
}

struct sample_t {
  sample_t(const unsigned int *samp_mem_): is_sd(false), sample_mem(samp_mem_) {}
  sample_t(const char *fn_): is_sd(true), filename(fn_) {}
  sample_t(): is_sd(false), sample_mem(0) {}
  
  bool is_sd;
  union {
    const unsigned int *sample_mem;
    const char *filename;
  };
};
struct sample_set_t {
  const int num_samples;
  sample_t samples[4];
};

sample_set_t metronome_sample_sets[] = {
  { 2, sample_t(AudioSampleTomtom), sample_t(AudioSampleSnare) },
  { 2, sample_t("STICK2.WAV"), sample_t("STICK1.WAV") },
  { 2, sample_t("WITTNER1.WAV"), sample_t("WITTNER2.WAV") }

};

int sample_set_idx = 1;

int playWav_idx = 0;
int playMem_idx = 0;
uint8_t sdi_d[1] = {0};

void update_shift()
{
  for (int i = 0; i < 8; i ++) {
    digitalWrite(PIN_SDI, (sdi_d[0] >> (7 - i)) & 0x1);
    digitalWrite(PIN_CLK, 1);
    digitalWrite(PIN_CLK, 0);
  }
  digitalWrite(PIN_STROBE, 1);
  digitalWrite(PIN_STROBE, 0);
}

int vol_count = 16;
float prev_gain;
bool first_gain = true;
float prev_bpm;
bool first_bpm = true;
bool amp_disabled = true;

void loop()
{
  if (amp_disabled && millis() > 1000 && beat == 0) {
    audioamp.enableChannel(true, true);
    amp_disabled = false;
  }

  if (beat_timer >= beat_timer_ms) {
    beat_timer = 0;

    const sample_set_t &samp_set = metronome_sample_sets[sample_set_idx];
    const sample_t &samp = samp_set.samples[(beat == 0) ? 0 : (1 + (beat % (samp_set.num_samples-1)))];

    if (samp.is_sd) {
      playWav[playWav_idx].play(samp.filename);
      playWav_idx = 1 - playWav_idx;
    }
    else {
      playMem[playMem_idx].play(samp.sample_mem);
      playMem_idx = 1 - playMem_idx;
    }

    //Serial.println("beat");

    //sdi_d[0] = ~((beat) | ((beat&1) << 7));
    static int tmp_cnt;
    sdi_d[0] = ~(tmp_cnt | ((beat&1) << 7));
    tmp_cnt = (tmp_cnt + 1)%10;
    
    update_shift();
      
    beat = (beat + 1) % meter_denom;
  }
  
  if (pot_timer >= 10) {
    pot_timer = 0;

    const int adc_vol = adc->adc0->analogRead(Pin_vol); 
    const float gain = 2.0 * (1.0 - (float)adc_vol / (float)adc->adc0->getMaxValue());

    if (first_gain || fabs(prev_gain - gain) > 0.01) {
      Serial.print("gain "); Serial.println(gain);
      amp1.gain(gain);
#if 0
      int pwm=min(255, 255*(1.0 - (float)adc_vol / (float)adc->adc0->getMaxValue()));
      analogWrite(Pin_pwm, pwm);
      Serial.print("pwm "); Serial.println(pwm);
#endif
      first_gain = false;
      prev_gain = gain;
    }
  
    const int adc_bpm = adc->adc1->analogRead(Pin_bpm);
    const float bpm_raw = (MAX_BPM - MIN_BPM) * (1.0 - (float)adc_bpm / (float)adc->adc1->getMaxValue()) + MIN_BPM;
    float bpm = fir.processReading(bpm_raw);
    if (vol_count) {
      vol_count --;
      bpm = bpm_raw;  // avoid filter until initialized 
    }
    
    if (first_bpm || fabs(prev_bpm - bpm) >= 0.2) {
      beat_timer_ms = int(60*1000.0 / bpm + 0.5);
      
      Serial.print("bpm "); Serial.println(bpm);
      first_bpm = false;
      prev_bpm = bpm;
    }

  }
  
  if (button.update() && button.fallingEdge()) {
    sample_set_idx = (sample_set_idx + 1) % (sizeof(metronome_sample_sets)/sizeof(metronome_sample_sets[0]));
    Serial.print("button "); Serial.println(sample_set_idx);
  }

}
