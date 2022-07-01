
static const int PIN_TRIGGER = 10; // active low
static const int PIN_CMPLO = 9;
static const int PIN_CMPHI = 8;
static const int PIN_CLKOUT = 11; // timer 2 output
static const int PIN_CLKIN = 5; // timer 1 input counter

void setup() {
  Serial.begin(9600);
 pinMode(PIN_TRIGGER, OUTPUT);
 pinMode(PIN_CMPLO, INPUT);
 pinMode(PIN_CMPHI, INPUT);
 pinMode(PIN_CLKOUT, OUTPUT);
 pinMode(PIN_CLKIN, INPUT);
 digitalWrite(PIN_TRIGGER, 1);
 digitalWrite(PIN_CLKOUT, 0);
 
 TCCR1A=0; // setup timer-1 (clk input counter)
  TCCR1C=0;
  TIMSK1=0;
  GTCCR=0;

    // Configure timer 2 .
    clkout_off();
    TCCR2B = 0
           | _BV(CS20);   // clock @ F_CPU / 1
    OCR2A  = 0 ;          // period 
    OCR2B  = 0;           
    TIMSK2 = 0;
  
 delay(100);
}

void clkout_on()
{
 TCCR2A = _BV(COM2A0)  // toggle mode on OC2A
           | _BV(WGM21)   // mode 2: CTC, TOP = OCR2A
           ; 
}

void clkout_off()
{
TCCR2A = 0  //  OC2A off
           | _BV(WGM21)   // mode 2: CTC, TOP = OCR2A
           ; 

}

#define NUM_TAPS 9

float coeff[NUM_TAPS] = { 
#if NUM_TAPS == 5
  0.078443841837769424,
0.253849604603263890,
0.335413107117933429,
0.253849604603263890,
0.078443841837769424
#endif

#if NUM_TAPS == 9
0.039899882278863812,
0.086079154232428304,
0.129118731348642463,
0.159599529115455191,
0.170605406049220448,
0.159599529115455191,
0.129118731348642463,
0.086079154232428304,
0.039899882278863812
#endif
};

float values[NUM_TAPS] = {0};

void loop() {
  // put your main code here, to run repeatedly:

  long start = millis();
  digitalWrite(PIN_TRIGGER, 0);

  delayMicroseconds(100); // skip glitch
  TCCR1B=7; // start counting
  clkout_on();
  unsigned long us_start = micros();
  if (digitalRead(PIN_CMPLO) == 0 || digitalRead(PIN_CMPHI) == 0) {
    Serial.println("Bad start");
  }
  while (digitalRead(PIN_CMPLO) == 1) {
    if (millis() - start > 100) {
      Serial.println("LO not found");
      break;
    }
  }
  unsigned long us_lo  = micros();
  if (digitalRead(PIN_CMPHI) == 0) {
    Serial.println("Bad HI start");
  }
  while (digitalRead(PIN_CMPHI) == 1) {
    if (millis() - start > 200) {
      Serial.println("HI not found");
      break;
    }
  }
  unsigned long us_hi = micros();
  TCCR1B=0; // stop
  clkout_off();

  uint16_t counts = TCNT1;
  TCNT1=0;
  digitalWrite(PIN_TRIGGER, 1);
   long delta = us_hi - us_lo;
//   if (delta < 0) {
//     delta = (0xffffffff - us_hi) - us_lo;
//   }
  float v = 10.0 - 10.0*((float)counts - 49000.0) / (54500.0 - 49000.0);
  for (int i = 1; i < NUM_TAPS; i++) {
    values[i] = values[i-1];
  }
  values[0] = v;
  float avg_v = 0;
  for (int i = 0; i < NUM_TAPS; i++) {
    avg_v += values[i] * coeff[i];
  }
   //Serial.print(counts); Serial.print(" "); // Serial.print(delta); Serial.print(" "); Serial.println(us_hi-us_start);
 
  Serial.print("v:"); Serial.print(v); Serial.print(", avg:"); Serial.println(avg_v);
  delay(100);
  
}
