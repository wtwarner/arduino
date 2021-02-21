// Teensy 3.2

#include <Audio.h>
#include <Wire.h>
#include "global.h"
#include "Yin.h"
#include "Yin16.h"
#include <AudioTuner.h>

const int DECIMATE_FACTOR = 4;
const int NUM_BUFFERS = 18;

// GUItool: begin automatically generated code
AudioInputAnalog         adc1(A2);           //xy=153,113
AudioFilterBiquad        biquad1;
//AudioAnalyzeNoteFrequency notefreq1;      //xy=351,110
AudioTuner               notefreq1;
AudioRecordQueue         queue1;         //xy=332,215
//AudioConnection          patchCord1(adc1, notefreq1);
AudioConnection          patchCord3(adc1, biquad1);
AudioConnection          patchCord2(biquad1, queue1);
AudioConnection           patchCord4(biquad1, notefreq1);
// GUItool: end automatically generated code

Yin yin(AUDIO_SAMPLE_RATE_EXACT / (float)DECIMATE_FACTOR, 128 * NUM_BUFFERS / DECIMATE_FACTOR);

Yin16 yin16;

// For AudioTuner, use decimation_Factor 4
int16_t fir_11029_HZ_coefficients[22] =
{
    0, 3, 6, -11, -71, 21,
    352, -15, -1202, -6, 5011, 8209,
    5011, -6, -1202, -15, 352, 21,
    -71, -11, 6, 3
};

void setup() {
  AudioMemory(30);

  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("start..");
  // put your setup code here, to run once:
  notefreq1.begin(.15, fir_11029_HZ_coefficients, sizeof(fir_11029_HZ_coefficients), DECIMATE_FACTOR);
  
  digitalWriteFast(LED_BUILTIN, 1);

  Yin_init(&yin16, 128 * NUM_BUFFERS / DECIMATE_FACTOR, 0.05, AUDIO_SAMPLE_RATE_EXACT / DECIMATE_FACTOR);
  biquad1.setLowpass(0, 20000/DECIMATE_FACTOR, .7);
  queue1.begin();
}

float autocor(int16_t *rawData, int len) {

  int thresh = 0;
  long sum = 0;

  byte pd_state = 0;
  int period = 0;
  const int OFFSET = 0;
  const int SCALE = 65536;
  for (int i = 0; i < len; i++)
  {
    // Autocorrelation
    long sum_old = sum;
    sum = 0;
    for (int k = 0; k < len - i; k++) sum += (rawData[k] - OFFSET) * (rawData[k + i] - OFFSET) / SCALE;
    // RX8 [h=43] @1Key1 @0Key1
    //    Serial.print("C");
    //    Serial.write((rawData[i]-128)>>8);
    //    Serial.write((rawData[i]-128)&0xff);
    //
    //    // RX8 [h=43] @1Key1 @0Key1
    //    Serial.print("C");
    //    Serial.write(sum>>8);
    //    Serial.write(sum&0xff);
    //
    // Peak Detect State Machine
    if (pd_state == 2 && (sum - sum_old) <= 0)
    {
      period = i;
      pd_state = 3;
    }
    if (pd_state == 1 && (sum > thresh) && (sum - sum_old) > 0) pd_state = 2;
    if (!i) {
      thresh = sum * 0.5;
      pd_state = 1;
    }
  }
  // Frequency identified in kHz
  return (float)AUDIO_SAMPLE_RATE_EXACT / DECIMATE_FACTOR / period;

}

int16_t mem[128 * NUM_BUFFERS / DECIMATE_FACTOR];
float mem_fp[128 * NUM_BUFFERS / DECIMATE_FACTOR];

#if 1
void loop()
{
    while (!notefreq1.available())
        ;
    Serial.printf("NF %03.2f\n", notefreq1.read(), notefreq1.probability());
}
#endif  
void loop1() {
  // put your main code here, to run repeatedly:
  //delay(5000);

  //Serial.printf("xx1\n");
  queue1.begin();
  while (queue1.available() < NUM_BUFFERS) 
    ; // wait
  
  for (int b = 0; b < NUM_BUFFERS; b ++) {
    int16_t *bfr = queue1.readBuffer();
    //memcpy(mem + b * 128, bfr, 128 * sizeof(int16_t));
    for (int s=0; s<128;  s += DECIMATE_FACTOR, bfr += DECIMATE_FACTOR) {
      mem[(b*128 + s) / DECIMATE_FACTOR] = *bfr;
      mem_fp[(b*128 +s ) / DECIMATE_FACTOR] = *bfr / 32768.0;
    }
    queue1.freeBuffer();
  }
  queue1.end();
  
 Serial.println("");
#if 0
  long start = millis();
  float f = autocor(mem, 128 * NUM_BUFFERS / DECIMATE_FACTOR);
  long duration = millis() - start;
  Serial.printf("AUTO Note %3.2f (time %ld)\n", f, duration);
#endif

 #if 0
  start = millis();
  float yin_f = yin.getPitch(mem_fp);
  float yin_prob = yin.getProbability();
  duration = millis()  - start;
  Serial.printf("YIN Note %3.2f, prob %2f (time %ld)\n", yin_f, yin_prob, duration);
 #endif
 #if 1
 long start = millis();
  float yin16_freq = Yin_getPitch(&yin16, mem);  
  float yin16_prob = Yin_getProbability(&yin16);
  long duration = millis()  - start;
  Serial.printf("YIN16 Note %3.2f, prob %2f (time %ld)\n", yin16_freq, yin16_prob, duration);
#endif

  digitalWriteFast(LED_BUILTIN, !digitalReadFast(LED_BUILTIN));

}
