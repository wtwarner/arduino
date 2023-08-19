#include "homefans.h"
#include "cc1101_debug_service.h"
#include <elapsedMillis.h>

elapsedMillis toggle_timer;
elapsedMillis led_timer;
byte led_state = 0;

void transmitState(int fanId, char* attr, char* payload) {
  ELECHOUSE_cc1101.SpiStrobe(CC1101_STX);                 // set Transmit on
  delay(50);
  mySwitch.enableTransmit(TX_PIN);          // Transmit on
  
  
  // Generate and send the RF payload to the fan
  int rfCommand = generateCommand(fanId, attr, payload);
  mySwitch.send(rfCommand, 12);
  mySwitch.disableTransmit();             // set Transmit off
  ELECHOUSE_cc1101.SpiStrobe(CC1101_SFSTXON);    // standby
  
}


int generateCommand(int fanId, char* attr, char* payload) {
  int baseCommand = 0b000011000000;
  int fanIdDips   = fanId << 8;
  int command     = 0b000000;

  if(strcmp(attr, "light") == 0) { // Handle "Fan Light" commands
    if(strcmp(payload, "toggle") == 0) {
      command = 0b111110;
    } 
  }
  else if(strcmp(attr, "fan") == 0) { // Handle "Fan Light" commands
    if(strcmp(payload, "off") == 0) {
      command = 0b111101;
    } 
    else if(strcmp(payload, "low") == 0) {
      command = 0b110111;
    } 
    else if(strcmp(payload, "med") == 0) {
      command = 0b101111;
    } 
    else if(strcmp(payload, "high") == 0) {
      command = 0b011111;
    } 
    else if(strcmp(payload, "rev") == 0) {
      command = 0b111011;
    } 
  } 
  else { // Handle all other commands (i.e. unknown commands)
    Serial.print("Unsupported command: (attr: ");
    Serial.print(attr);
    Serial.print(") ");
    Serial.println(payload);
  }

  // Combine all values together to create our final command
  int finalCommand = baseCommand | fanIdDips | command;
  return finalCommand;
}

void setup() {
  Serial.begin(9600);

  //pinMode(LED_BUILTIN, OUTPUT);
  
  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.setMHZ(RF_FREQUENCY);
  ELECHOUSE_cc1101.SetTx();
  ELECHOUSE_cc1101.SpiStrobe(CC1101_SFSTXON);                 // standby

  mySwitch.setRepeatTransmit(RF_REPEATS);   // set RF code repeat
  mySwitch.setProtocol(RF_PROTOCOL);        // send Received Protocol
  mySwitch.setPulseLength(RF_PULSE_LENGTH); // modify this if required

  delay(1000);
  Serial.print("fan off\n");
  transmitState(0, "fan", "off");
}

void loop() {
  cc1101_debug.debug ();
  if (toggle_timer >= 2000) {
    Serial.print("light toggle\n");
    transmitState(0, "light", "toggle");
    toggle_timer = 0; 
  }
  if (led_timer >= 500) {
    //digitalWrite(LED_BUILTIN, led_state);
    led_state = !led_state;
    led_timer = 0;
  }
}
