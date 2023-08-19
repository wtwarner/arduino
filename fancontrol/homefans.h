#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <RCSwitch.h>
#include "config.h"

// Set receive and transmit pin numbers (GDO0 and GDO2)
#ifdef ESP32 // for esp32! Receiver on GPIO pin 4. Transmit on GPIO pin 2.
  #define RX_PIN 4 
  #define TX_PIN 2
#elif ESP8266  // for esp8266! Receiver on pin 4 = D2. Transmit on pin 5 = D1.
  #define RX_PIN 4
  #define TX_PIN 5
#else // for Arduino! Receiver on interrupt 0 => that is pin #2. Transmit on pin 6.
  #define RX_PIN 0
  #define TX_PIN 6
#endif


RCSwitch mySwitch = RCSwitch();
