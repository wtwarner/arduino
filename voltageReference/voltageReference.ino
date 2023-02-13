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
 #include "PID_AutoTune_v0.h"
 #include "elapsedMillis.h"
 
 double pid_setpoint, pid_input, pid_output=128;
 PID heaterPID(&pid_input, &pid_output, &pid_setpoint, 3, 1, .2, DIRECT);
 PID_ATune heaterPIDAtune(&pid_input, &pid_output);
 
 void setup() {
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  analogReference(INTERNAL); // 1.1V
  pinMode(A7, INPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  digitalWrite(4, 1);
  digitalWrite(3, 0);
  
  pid_input = 10.0; // degrees C
  pid_setpoint = 30.0; // degrees C
  heaterPID.SetSampleTime(10);
  heaterPID.SetMode(AUTOMATIC);
}

int cnt = 0;
bool tuned = false;
elapsedMillis startTuning;
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
    raw_v = analogRead(A7);
  //float v = raw_v/1024.0*1.1;
  //float deg_c = (v - .400) / .0196; // MCP9701 
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
  digitalWrite(3, 0);
  delayMicroseconds(10);
 
  #if 1
   int32_t deg_c = read_mcp9701(); 
  #else
  int32_t deg_c = 25*65536;
  #endif
  
  pid_input = deg_c;
  #if 1
  int32_t inttemp_c = read_inttemp();
  #else
  int32_t inttemp_c = 25;
  #endif
  //heaterPID.Compute();
  //float deg_f = deg_c*9.0/5.0 + 32.0;
  
  
//    if (!tuned && startTuning > 10000) {
//       
//      if (heaterPIDAtune.Runtime())
//      {
//        tuned = true;
//        //we're done, set the tuning parameters
//        float kp = heaterPIDAtune.GetKp();
//        float ki = heaterPIDAtune.GetKi();
//        float kd = heaterPIDAtune.GetKd();
//        //heaterPID.SetTunings(kp,ki,kd);
//        
//          Serial.println("tuned:");
//          Serial.println(kp); Serial.println(ki); Serial.println(kd);
//      }
//    }
        
    //analogWrite(3, (int)pid_output);
  delayMicroseconds(255-(int)pid_output);
  if (inttemp_c < 3276800) // 50c in s16.16
    digitalWrite(3, 1);
  delayMicroseconds((int)pid_output);

  if (++cnt == 250) {
    
     Serial.print(pid_output); Serial.print(" "); Serial.print(" "); Serial.print(deg_c); Serial.print(" "); Serial.print(deg_c/65536.0);Serial.print(" "); Serial.println(inttemp_c/65536.0); 
     cnt = 0;

  }
  
  // watchdog
  if (watchdog > 500) {
    digitalWrite(4, 0);
  }
  if (watchdog > 502) {
    digitalWrite(4, 1);
    watchdog = 0;
  }
}
