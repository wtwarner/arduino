/*
 *  Arduino Nano
 *  Must chagne Library/Arduino15/packages/arduino/hardware/avr/1.8.6/cores/arduino/wiring_analog.c
 *  to use 4-bit mask for ADMUX:
 *          ADMUX = (analog_reference << 6) | (pin & 0x0f);
 * temperature;
 * MCP9701
 * 0 degrees C: 400 mV
 * 19.6 mV/degreeC
 * 
 */
 #include "PID_v1.h"
 #include "elapsedMillis.h"

 const int PIN_REACHTEMP_LED = A1;
 const int PIN_TEMP = A7; // MCP9701 output
 const int PIN_HEATER_PWM = 3; // other LED is on this (after AND with Watchdog)
 const int PIN_HEATER_WATCHDOG = 4; // pulse to 555 reset
 
 double pid_setpoint, pid_input, pid_output=0;
 PID heaterPID(&pid_input, &pid_output, &pid_setpoint, .5, .1, 0, DIRECT);
 
 void setup() {
  Serial.begin(38400);

  analogReference(INTERNAL); // 1.1V
  pinMode(PIN_TEMP, INPUT);
  
  pinMode(PIN_HEATER_PWM, OUTPUT);
  digitalWrite(PIN_HEATER_PWM, 0);
  
  pinMode(PIN_HEATER_WATCHDOG, OUTPUT);
  digitalWrite(PIN_HEATER_WATCHDOG, 1);
  
  pinMode(PIN_REACHTEMP_LED, OUTPUT);
  digitalWrite(PIN_REACHTEMP_LED, 0);
  
  pid_input = 25.0; // degrees C
  pid_setpoint = 30.0; // degrees C
  heaterPID.SetSampleTime(1);
  heaterPID.SetOutputLimits(0.0, 1000.0);
  heaterPID.SetMode(AUTOMATIC);
}

int cnt = 0;
elapsedMillis watchdog;
/*
 * (((744/1024*1.1*65536) - .4*65536)/.0196)/65536
 * (1/1024*1.1*65536) = 70.40000000000000000000
   .4*65536/.0196 = 1337469.38

 */
int32_t read_mcp9701()
{
  int32_t raw_v;
  for (int i = 0; i < 4; i++)
    raw_v = analogRead(PIN_TEMP);

  int32_t deg_c = (raw_v) * 3592 - 1337469;
  //Serial.print(raw_v);Serial.print(" "); Serial.println( deg_c); delay(100);
  return deg_c;
}

/*
 * read internal temperature of ATMega328 using ADC channel 8.
 * Assumes internal reference of 1.1V.
 * measured ADC offset at room temp of 21.6 C:  326
 * nominal temp coeff: 1 mV/C
 */
int32_t read_inttemp()
{
     int raw_v;
     for (int i=0;i<3;i++) 
        raw_v = analogRead(8);
     // in s16.16: 1/1024.0*1.1/.001*65536 = 70400
     // in s16.16: 21.6*65536 = 1415578
     int32_t c = (raw_v-326)*70400 + 1415578;
     //float c = ((raw_v-326)/1024.0*1.1) / .001 + 21.6;
     //float c = 1.0;
     return c;
}


void loop() {
  long startUs = micros();

  // quiese
  delayMicroseconds(10);
 
  int32_t deg_c = read_mcp9701(); 
  
  pid_input = (float)deg_c/65536.0;
  int32_t inttemp_c = read_inttemp();

  heaterPID.Compute();
  const int pid_output_int = (int)(pid_output + 0.5);

  // do manual pulse width modulation
  delayMicroseconds(1000-pid_output_int);

  // turn on, but only if internal temp is safe
  if ((int)pid_output > 0 && inttemp_c < 3276800) { // 50c in s16.16
    digitalWrite(PIN_HEATER_PWM, 1);
    delayMicroseconds(pid_output_int);
  }
  // turn off
  digitalWrite(PIN_HEATER_PWM, 0);

  // LED: open-drain, drive low to turn on.  Tristate to turn off.
  // On if temp is between 29.5 and 30.5 (in s16.16)
  const bool temp_led = (deg_c >= 1933312) && (deg_c <= 1998848);
  if (temp_led) {
    pinMode(PIN_REACHTEMP_LED, OUTPUT);
    digitalWrite(PIN_REACHTEMP_LED, 0);
  }
  else {
    pinMode(PIN_REACHTEMP_LED, INPUT);
  }

  if (++cnt == 100) {
     Serial.print(pid_output_int); Serial.print(" "); Serial.print(deg_c); Serial.print(" "); Serial.print(deg_c/65536.0);Serial.print(" "); Serial.println(inttemp_c/65536.0); 
     cnt = 0;
  }
  
  // watchdog reset pulse to 555 timer
  if (watchdog > 500) {
    digitalWrite(PIN_HEATER_WATCHDOG, 0);
  }
  if (watchdog > 502) {
    digitalWrite(PIN_HEATER_WATCHDOG, 1);
    watchdog = 0;
  }
    
  
}
