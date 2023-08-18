#include "homefans.h"

void transmitState(int fanId, char* attr, char* payload) {
  ELECHOUSE_cc1101.SetTx();                 // set Transmit on
  mySwitch.disableReceive();                // Receiver off
  mySwitch.enableTransmit(TX_PIN);          // Transmit on
  mySwitch.setRepeatTransmit(RF_REPEATS);   // set RF code repeat
  mySwitch.setProtocol(RF_PROTOCOL);        // send Received Protocol
  mySwitch.setPulseLength(RF_PULSE_LENGTH); // modify this if required
  
  // Generate and send the RF payload to the fan
  int rfCommand = generateCommand(fanId, attr, payload);
  mySwitch.send(rfCommand, 12);

  
  #if DEBUG_MODE
    Serial.print("(RF) OUT [protocol: ");
    Serial.print(RF_PROTOCOL);
    Serial.print("] [repeats: ");
    Serial.print(RF_REPEATS);
    Serial.print("] [frequency: ");
    Serial.print(RF_FREQUENCY);
    Serial.print("] [fan: ");
    Serial.print(fanId);
    Serial.print("] [attr: ");
    Serial.print(attr);
    Serial.print("] [payload: ");
    Serial.print(payload);
    Serial.print("] ");
    Serial.print(rfCommand);
    Serial.println();
  #endif
  
  ELECHOUSE_cc1101.SetRx();               // set Receive on
  mySwitch.disableTransmit();             // set Transmit off
  mySwitch.enableReceive(RX_PIN);         // Receiver on
  

}


int generateCommand(int fanId, char* attr, char* payload) {
  int baseCommand = 0b000011000000;
  int fanIdDips   = fanId << 8;
  int command     = 0b000000;

  if(strcmp(attr, "light") == 0) { // Handle "Fan Light" commands
    if(strcmp(payload, "toggle") == 0) {
      command = 0b111110;
    } 
  } else { // Handle all other commands (i.e. unknown commands)
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
  
  // initialize fan struct
  for(int i=0; i<16; i++) {
    fans[i].lightState = false;
    fans[i].fanState = false;
    fans[i].fanSpeed = 0;
  }
  
  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.setMHZ(RF_FREQUENCY);
  ELECHOUSE_cc1101.SetRx();
  mySwitch.enableReceive(RX_PIN);
  
}

void loop() {
  transmitState(0, "light", "toggle");
  delay(2000);
}

// Fin.
