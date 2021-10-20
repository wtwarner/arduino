//
// metronome
// for Teensy 4.0 with Audio Adapter and Adafruit TPA2016 board.
//

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Bounce2.h>
#include <ADC.h>
#include <ADC_util.h>
#include <FIR.h>
#include "Adafruit_TPA2016.h"

// WAV files converted to code by wav2sketch
#include "AudioSampleSnare.h"        // http://www.freesound.org/people/KEVOY/sounds/82583/
#include "AudioSampleTomtom.h"       // http://www.freesound.org/people/zgump/sounds/86334/

//
// Pin assignments
//
static const int PIN_BPM = 14; // ADC0 - bpm
static const int PIN_VOL = 15; // ADC1 - volume

static const int PIN_BUT1 = 6;    // button 1
static const int PIN_BUT2 = 5;    // button 2
static const int PIN_SD_CS = 10; // SD Card

static const int PIN_CLK = 2;   // Nixie shift register
static const int PIN_SDI = 0;
static const int PIN_STROBE = 1;

static const int PIN_PWM = 9;   // Nixie brightness PWM

//
// Audio processing
//
AudioPlayMemory    playMem;
AudioPlaySdWav     playWav[2];
AudioSynthWaveform playTone;

AudioMixer4        mix1;    //  4-channel mixer
AudioAmplifier     amp1;
AudioOutputI2S     lineout;

AudioConnection c1(playMem, 0, mix1, 0);
AudioConnection c3(playWav[0], 0, mix1, 1);
AudioConnection c4(playWav[1], 0, mix1, 2);
AudioConnection c2(playTone, 0, mix1, 3);

AudioConnection c5(mix1, 0, amp1, 0);

AudioConnection c8(amp1, 0, lineout, 0);
AudioConnection c9(amp1, 0, lineout, 1);

//
// Audio Shield Codec
//
AudioControlSGTL5000 audioShield;

//
// Audio Amp
//
Adafruit_TPA2016 audioamp = Adafruit_TPA2016();

//
// ADC for potentiometers
//
ADC *adc = new ADC(); // adc object;

//
// ADC Filter
//
/*

  FIR filter designed with
  http://t-filter.appspot.com

  sampling frequency: 250 Hz

  0 Hz - 10 Hz
  gain = 1
  desired ripple = 2 dB
  actual ripple = 1.514479888060941 dB

  20 Hz - 125 Hz
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

Bounce2::Button button1, button2;

//
// persistent state
//
struct persistent_state_t {
  persistent_state_t():
    dev_p((uint32_t *)0x400d4100) // IMXRT_SNVS_LPGPR0
  {
    SNVS_LPCR |= (1 << 24); //Enable NVRAM - documented in SDK
  }

  union {
    struct {
      uint8_t sample_set_idx;
      uint8_t nixie_brightness;
      uint8_t meter_denom;
      uint8_t reserved[12];
      uint8_t checksum;
    } s;
    uint8_t d[16];
    uint32_t d32[4];
  };
  uint32_t *dev_p;

  void init() {
    memset(d, 0, sizeof(d));
    s.sample_set_idx = 1;
    s.nixie_brightness = 192;
    s.meter_denom = 4;
    update_checksum();
  }

  void update_checksum() {
    uint8_t cs = 255;
    for (unsigned int i = 0; i < sizeof(d) - 1; i++) {
      cs += d[i];
    }
    s.checksum = cs;

    dev_p[0] = d32[0];
    dev_p[1] = d32[1];
    dev_p[2] = d32[2];
    dev_p[3] = d32[3];

  }

  bool verify_checksum()  {
    d32[0] = dev_p[0];
    d32[1] = dev_p[1];
    d32[2] = dev_p[2];
    d32[3] = dev_p[3];

    uint8_t cs = 255;
    for (unsigned int i = 0; i < sizeof(d) - 1; i++) {
      cs += d[i];
    }
    return cs == s.checksum;
  }
};

persistent_state_t pst;

static const float MAX_BPM = 400.0;
static const float MIN_BPM = 10.0;
static const int MAX_METER_DENOM = 12;
int beat = 0;
float bpm = 120.0;
float beat_timer_ms = 500.0; // 120 bpm
elapsedMillis beat_timer;

elapsedMillis pot_timer;

enum { ENTRY_BPM, ENTRY_BRIGHTNESS, ENTRY_DENOM, ENTRY_TONE } entry_mode = ENTRY_BPM;

void dump()
{
  Serial.printf("d  3: 0: %3d.%3d.%3d.%3d\n", pst.d[3], pst.d[2], pst.d[1], pst.d[0]);
  Serial.printf("d  7: 4: %3d.%3d.%3d.%3d ...\n", pst.d[7], pst.d[6], pst.d[5], pst.d[4]);
  Serial.printf("d 15:12 %3d.%3d.%3d.%3d\n", pst.d[15], pst.d[14], pst.d[13], pst.d[12]);
  Serial.printf("s_idx %d, bpm %d,  denom %d, chksum %d\n",
                pst.s.sample_set_idx,
                pst.s.nixie_brightness,
                pst.s.meter_denom,
                pst.s.checksum);

}

void setup() {
  delay(1000);
  Serial.println("Start");

  if (!pst.verify_checksum()) {
    Serial.println("pst: bad checksum, init");
    pst.init();
  }
  else {
    Serial.println("pst: good checksum");
    Serial.print("denom "); Serial.println(pst.s.meter_denom);
    Serial.print("brightness "); Serial.println(pst.s.nixie_brightness);
  }

  pinMode(PIN_BPM, INPUT);
  pinMode(PIN_VOL, INPUT);
  pinMode(PIN_PWM, OUTPUT);
  analogWriteFrequency(PIN_PWM, 500);
  analogWrite(PIN_PWM, pst.s.nixie_brightness);

  button1.attach(PIN_BUT1, INPUT);
  button1.setPressedState(LOW);
  button2.attach(PIN_BUT2, INPUT);
  button2.setPressedState(LOW);

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
  audioamp.enableChannel(false, false);
  audioamp.setAGCCompression(TPA2016_AGC_OFF);
  audioamp.setReleaseControl(0);
#if 0
  // "Rock" settings: ratio 2:1, attack 3.84 ms/6db, release 4110 ms/6db, hold --, fixed gain 6, limiter 8
  audioamp.setAGCCompression(TPA2016_AGC_2);
  audioamp.setAttackControl(int(3.84 / 1.28 + 0.5));
  audioamp.setReleaseControl(int(0 / 1644 + 0.5));
  audioamp.setHoldControl(int(0/*.2740*/ / .0137 + 0.5));
#endif
  audioamp.setGain(20);

  AudioMemory(10);

  audioShield.enable();
  audioShield.volume(.5);     // headphone only; no effect on line out

  mix1.gain(0, 1.0);
  mix1.gain(1, 1.0);
  mix1.gain(2, 1.0);
  mix1.gain(3, 0.0); // tone off
  
  amp1.gain(0.0);

  if (!SD.begin(PIN_SD_CS)) {
    Serial.println("Unable to access SD card");
  }

  fir.setFilterCoeffs(filter_taps);

  update_shift();
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
static const unsigned int num_sample_sets = sizeof(metronome_sample_sets) / sizeof(metronome_sample_sets[0]);

//
// for two TPIC6B595 shift registers
//
uint8_t sdi_d[2] = {0, 0};

void update_shift()
{
  for (int i = 0; i < 16; i ++) {
    digitalWrite(PIN_SDI, (~sdi_d[i / 8] >> (7 - (i % 8))) & 0x1);
    digitalWrite(PIN_CLK, 1);
    digitalWrite(PIN_CLK, 0);
  }
  digitalWrite(PIN_STROBE, 1);
  digitalWrite(PIN_STROBE, 0);
}

int vol_count = 35;
float prev_gain;
bool first_gain = true;
float prev_bpm;
bool first_bpm = true;
bool amp_disabled = true;

uint8_t note_idx = 0;
const float note_freq[12] = { 
  110.00 , // A2
 116.54,
 123.47 ,
  130.81,
138.59 ,
 146.83,
 155.56,
 164.81,
 174.61,
  185.00 ,
 196.00 ,
  207.65 //  G#3/Ab3
};

// indices to alternate between two players in case they overlap in time.
int playWav_idx = 0;

elapsedMillis button1_time, button2_time;

void loop()
{
    bool shift = false;

    if (amp_disabled && millis() > 1000 && beat == 0) {
        audioamp.enableChannel(true, true);
        amp_disabled = false;
    }

    if (beat_timer >= beat_timer_ms) {
        beat_timer = 0;

        const sample_set_t &samp_set = metronome_sample_sets[pst.s.sample_set_idx];
        const sample_t &samp = samp_set.samples[(beat == 0) ? 0 : (1 + (beat % (samp_set.num_samples - 1)))];

        if (samp.is_sd) {
            playWav[playWav_idx].play(samp.filename);
            playWav_idx = 1 - playWav_idx;
        }
        else {
            playMem.play(samp.sample_mem);
        }

        //Serial.println("beat");

        sdi_d[1] = (sdi_d[1] & ~0x80) | ((beat & 1) << 7);
        shift = true;

        beat = (beat + 1) % pst.s.meter_denom;
    }

    if (pot_timer >= 10) {
        pot_timer = 0;

        //
        // volume pot
        //
        const int adc_vol = adc->adc0->analogRead(PIN_VOL);
        const float gain = 2.0 * (1.0 - (float)adc_vol / (float)adc->adc0->getMaxValue());

        if (first_gain || fabs(prev_gain - gain) > 0.01) {
            Serial.print("gain "); Serial.println(gain);
            amp1.gain(gain);
            first_gain = false;
            prev_gain = gain;
        }

        //
        // BPM pot
        //
        const int adc_bpm = adc->adc1->analogRead(PIN_BPM);
        const float bpm_raw = (MAX_BPM - MIN_BPM) * (1.0 - (float)adc_bpm / (float)adc->adc1->getMaxValue()) + MIN_BPM;
        float bpm_filt = fir.processReading(bpm_raw);
        if (vol_count) {
            vol_count --;
            bpm_filt = bpm_raw;  // avoid filter until initialized
        }
    
        if (first_bpm || fabs(prev_bpm - bpm_filt) >= 0.2) {
            bpm = bpm_filt;
            beat_timer_ms = int(60 * 1000.0 / bpm + 0.5);
            //Serial.print("bpm "); Serial.println(bpm);
            encode_decimal(bpm);
            shift = true;
        
            first_bpm = false;
            prev_bpm = bpm;
        }
    }

    //
    // Normal mode:
    //   button 1 or 2: decrease/increase sample set
    // button 1+2 press: enter Brightness
    //   button 1 or 2: decrease/increase
    // button 1+2 press: enter Denom
    //   button 1 or 2: decrease/increase
    button1.update();
    button2.update();
    static bool swallow1 = false;
    static bool swallow2 = false;
  
     bool push1 = button1.released();
     if (push1 && swallow1) { push1 = swallow1 = false; }
     bool push2 = button2.released();
     if (push2 && swallow2) { push2 = swallow2 = false; }
     
    const bool both_buttons = button1.pressed() && button2.isPressed() ||
                button1.isPressed() && button2.pressed();

    switch (entry_mode) {
        case ENTRY_BPM:
            if (both_buttons) {
                entry_mode = ENTRY_TONE;
                swallow1 = swallow2 = true;
                playTone.frequency(note_freq[note_idx]*2);
                playTone.amplitude(1.0);
                playTone.begin(WAVEFORM_SINE);
                encode_decimal(note_idx);
                shift = true;
                for (int i = 0; i < 3; i ++) mix1.gain(i,0.4);
                mix1.gain(3, 0.8); // tone on

                Serial.println("entry: tone");
            }
            else if (push1 || push2) {
                // sample set idx wraps around 
                unsigned int delta = push1 ? (num_sample_sets-1) : 1;
                pst.s.sample_set_idx = (pst.s.sample_set_idx + delta) % num_sample_sets;
                pst.update_checksum();
                Serial.print("button: sample "); Serial.println(pst.s.sample_set_idx);
            }
            break;

       case ENTRY_TONE:
            if (both_buttons) {
                entry_mode = ENTRY_BRIGHTNESS;
                button1_time = 0;
                button2_time = 0;
                encode_decimal(bpm);
                shift = true;
                swallow1 = swallow2 = true;
                for (int i = 0; i < 3; i ++) mix1.gain(i,1.0);
                mix1.gain(3, 0.0); // tone off
                Serial.println("entry: brightness");
            }
            else if (push1 || push2) {
                // note idx wraps around 
                unsigned int delta = push1 ? (12-1) : 1;
                note_idx = (note_idx + delta) % 12;
                playTone.frequency(note_freq[note_idx]*2);
                encode_decimal(note_idx);
                shift = true;
                pst.update_checksum();
                Serial.print("button: tone "); Serial.println(note_idx);
            }
            break;
      
        case ENTRY_BRIGHTNESS: {
            int delta = 0;
            if (both_buttons) {
                entry_mode = ENTRY_DENOM;
                swallow1 = swallow2 = true;
                encode_decimal(pst.s.meter_denom);
                shift = true;
                Serial.println("entry: denom");
            }
            else if (push1) {
                delta = -1;
            }
            else if (button1.pressed()) {
                button1_time = 0;
            }
            else if (button1.isPressed() && button1_time > 250) {
                button1_time = 0;
                delta = -10;
            }
            else if (push2) {
                delta = +1;
            }
            else if (button2.pressed()) {
                button2_time = 0;
            }
            else if (button2.isPressed() && button2_time > 250) {
                button2_time = 0;
                delta = +10;
            }

            if (delta) {
                float new_b = pst.s.nixie_brightness + delta;
                pst.s.nixie_brightness = max(0, min(int(new_b + 0.5), 255));
                pst.update_checksum();

                analogWrite(PIN_PWM, pst.s.nixie_brightness);

                Serial.print("bright "); Serial.println(pst.s.nixie_brightness);
            }
            break;
        }

        case ENTRY_DENOM:
        {
            int delta = 0;
            if (both_buttons) {
                entry_mode = ENTRY_BPM;
                swallow1 = swallow2 = true;
                encode_decimal(bpm);
                shift = true;
                Serial.println("entry: bpm");
            }
            else if (push1) {
                delta = MAX_METER_DENOM - 1;
            }
            else if (push2) {
                delta = 1;
            }

            if (delta) {
                pst.s.meter_denom = (pst.s.meter_denom - 1 + delta) % MAX_METER_DENOM + 1;
                pst.update_checksum();

                encode_decimal(pst.s.meter_denom);
                shift = true;
                beat = beat % pst.s.meter_denom;

                Serial.print("denom "); Serial.println(pst.s.meter_denom);
            }
            break;
        }
    }

    if (shift) {
        update_shift();
    }
}

void encode_decimal(float val)
{
  const uint8_t BLANK = 15;
  const uint16_t val_int = int(val + 0.5);
  const uint8_t hundreds = val_int / 100;
  const uint8_t tens = (val_int % 100) / 10;
  const uint8_t ones = (val_int % 10);
  const bool blank_hundreds = (hundreds == 0);
  const bool blank_tens = blank_hundreds && (tens == 0);
  //Serial.printf("%g %d.%d.%d\n", val, hundreds, tens, ones);
  sdi_d[1] &= ~0xf;
  sdi_d[1] |= blank_hundreds ? BLANK : hundreds;
  sdi_d[0] = ((blank_tens ? BLANK : tens) << 4) | ones;
}
