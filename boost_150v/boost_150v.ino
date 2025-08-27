#if 0
  //Adduino Uno: pin 9
  //ItsyBitsy M0 Express: pin 5 (Vhi pin)

  Boost converter.
  IRF620 mosfet
  L: 3x20mH parallel * 2 serial: 1/(3/20) * 2 == 13 mH
  C; 3 * .47 uF film parallel = 1.5 uF
  optimal freq (by experiment): 2.5 kHz, duty 97%
  150kOhm load meter (2ma?): 5V in, ~104V out

  L charge pulse: 350 uS
  L discharge pulse: 14.6 uS ~104V

  === 2 ===
  L 1 mH
  C 2 * .47 
  5V in, ~136V out
  freq 42kHz, duty 97%
  L discharge: 800ns
  L charge: 22us
#endif

#ifdef __ARR__
#include "AVR_PWM.h"
AVR_PWM* PWM_Instance;
const int pin_pwm = 9; 
#else
#include "SAMD_PWM.h"
SAMD_PWM* PWM_Instance;
const int pin_pwm = 5; 
#endif

float freq = 42000;
float duty = 95;

void printPWMInfo(SAMD_PWM* PWM_Instance) 
 {  
   Serial.print("Actual data: pin = "); 
   Serial.print(PWM_Instance->getPin()); 
   Serial.print(", PWM DC = "); 
   Serial.print(PWM_Instance->getActualDutyCycle()); 
   Serial.print(", PWMPeriod = "); 
   Serial.print(PWM_Instance->getPWMPeriod()); 
   Serial.print(", PWM Freq (Hz) = "); 
   Serial.println(PWM_Instance->getActualFreq(), 4); 
 } 
  
void setup() {
  Serial.begin(9600);
  pinMode(pin_pwm, OUTPUT);
#ifdef __AVR__
  PWM_Instance = new AVR_PWM(pin_pwm, freq, duty);
#else
  PWM_Instance = new SAMD_PWM(pin_pwm, freq, duty, 0, 8);
#endif
  PWM_Instance->setPWM();
}

static bool ramp = 0;
static float ramp_inc = -1;
void loop() {
  // put your main code here, to run repeatedly:
   //PWM_Instance->setPWM(pin_pwm, 40000, 99);
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == 'f') {
      freq = Serial.parseFloat();
      ramp = false;
    }
    else if (c == 'd') {
      duty = Serial.parseFloat();
      ramp = false;
    }
    else if (c == 'r') {
      ramp = !ramp;
      Serial.print("ramp "); Serial.println(ramp);
    }
    PWM_Instance->setPWM(pin_pwm, freq, duty);
    Serial.print("freq "); Serial.print(freq);
    Serial.print(" duty "); Serial.println(duty);
    printPWMInfo(PWM_Instance);
    while ((c = Serial.read()), c != '\n') {}
  }
  if (ramp) {
    if (duty < 10 && ramp_inc < 0 || duty > 95 && ramp_inc > 0) { ramp_inc *= -1;}
    delay(200);
    duty += ramp_inc;
    PWM_Instance->setPWM(pin_pwm, freq, duty);
  }
}
