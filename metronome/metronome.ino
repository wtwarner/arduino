#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <ADC.h>
#include <ADC_util.h>

#include "Adafruit_TPA2016.h"

// WAV files converted to code by wav2sketch
#include "AudioSampleSnare.h"        // http://www.freesound.org/people/KEVOY/sounds/82583/
#include "AudioSampleTomtom.h"       // http://www.freesound.org/people/zgump/sounds/86334/
#include "AudioSampleHihat.h"        // http://www.freesound.org/people/mhc/sounds/102790/
#include "AudioSampleKick.h"         // http://www.freesound.org/people/DWSD/sounds/171104/

// Create the Audio components.  These should be created in the
// order data flows, inputs/sources -> processing -> outputs
//
AudioPlayMemory    sound0;
AudioPlayMemory    sound1;  // six memory players, so we can play
AudioPlayMemory    sound2;  // all six sounds simultaneously
AudioPlayMemory    sound3;  // all six sounds simultaneously

AudioMixer4        mix1;    //  4-channel mixer
AudioAmplifier     amp1;
AudioOutputI2S     lineout;

// Create Audio connections between the components
//
AudioConnection c1(sound0, 0, mix1, 0);
AudioConnection c2(sound1, 0, mix1, 1);
AudioConnection c3(sound2, 0, mix1, 2);
AudioConnection c4(sound3, 0, mix1, 3);

AudioConnection c5(mix1, 0, amp1, 0);

AudioConnection c8(amp1, 0, lineout, 0);
AudioConnection c9(amp1, 0, lineout, 1);

// Create an object to control the audio shield.
// 
AudioControlSGTL5000 audioShield;

Adafruit_TPA2016 audioamp = Adafruit_TPA2016();

const int readPin = 14; // ADC0
const int readPin2 = 15; // ADC1

ADC *adc = new ADC(); // adc object;


void setup() {
      pinMode(readPin, INPUT);
    pinMode(readPin2, INPUT);

   adc->adc0->setAveraging(32); // set number of averages
    adc->adc0->setResolution(12); // set bits of resolution

    // it can be any of the ADC_CONVERSION_SPEED enum: VERY_LOW_SPEED, LOW_SPEED, MED_SPEED, HIGH_SPEED_16BITS, HIGH_SPEED or VERY_HIGH_SPEED
    // see the documentation for more information
    // additionally the conversion speed can also be ADACK_2_4, ADACK_4_0, ADACK_5_2 and ADACK_6_2,
    // where the numbers are the frequency of the ADC clock in MHz and are independent on the bus speed.
    adc->adc0->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_LOW_SPEED); // change the conversion speed
    // it can be any of the ADC_MED_SPEED enum: VERY_LOW_SPEED, LOW_SPEED, MED_SPEED, HIGH_SPEED or VERY_HIGH_SPEED
    adc->adc0->setSamplingSpeed(ADC_SAMPLING_SPEED::LOW_SPEED); // change the sampling speed

   adc->adc1->setAveraging(32); // set number of averages
    adc->adc1->setResolution(12); // set bits of resolution
    adc->adc1->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_LOW_SPEED); // change the conversion speed
    adc->adc1->setSamplingSpeed(ADC_SAMPLING_SPEED::LOW_SPEED); // change the sampling speed

   audioamp.begin();
   // "Rock" settings: ratio 2:1, attack 3.84, release 4110, hold --, fixed gain 6, limiter 8
   
  audioamp.setAGCCompression(TPA2016_AGC_2);
  audioamp.setAttackControl(int(3.84/.1067));
  audioamp.setReleaseControl(int(.4110/.0137));
  audioamp.setHoldControl(0);
  audioamp.setGain(8);
  

  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(10);

  // turn on the output
  audioShield.enable();
  audioShield.volume(.5);

  // reduce the gain on mixer channels, so more than 1
  // sound can play simultaneously without clipping
  mix1.gain(0, 0.8);
  mix1.gain(1, 0.8);
  mix1.gain(2, 0.8);

  amp1.gain(0.5);

}

elapsedMillis beat_timer, vol_timer;
int beat = 0;
int bpm = 120;
int beat_timer_ms = 500;
int meter_denom = 4;

void loop() {

  if (beat_timer >= beat_timer_ms) {
    
      //if (1) sound1.play(AudioSampleHihat);
      if (beat == 0) sound2.play(AudioSampleTomtom);
      else sound0.play(AudioSampleSnare);
      beat = (beat + 1) % meter_denom;
      //Serial.println("beat");
    
    beat_timer = 0;
  }
  else if (beat_timer < (beat_timer_ms-1) && vol_timer >= 200) {
    int value = adc->adc0->analogRead(readPin); 
    float gain = 2.0 * (1.0 - (float)value / (float)adc->adc0->getMaxValue());
    //Serial.print("amp_gain "); Serial.println(gain);
    //audioamp.setGain(amp_gain);
    //audioShield.volume(gain);
    amp1.gain(gain);

    int value2 = adc->adc1->analogRead(readPin2);
    bpm = int(990.0 * (1.0 - (float)value2 / (float)adc->adc1->getMaxValue()) + 1) + 10;
    beat_timer_ms = (60*1000 + bpm/2) / bpm;

    vol_timer = 0;
  }
 
}
