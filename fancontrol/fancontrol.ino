#include "homefans.h"
#include "cc1101_debug_service.h"
#include <elapsedMillis.h>
#include <Bounce2.h>
extern byte sin8(int deg);

enum {
  BUTTON_LIGHT,
  BUTTON_FAN_OFF,
  BUTTON_FAN_LOW,
  BUTTON_FAN_MED,
  BUTTON_FAN_HIGH,
  BUTTON_FAN_REV,
  NUM_BUTTONS
};

Bounce2::Button button[NUM_BUTTONS];

const int button_pins[NUM_BUTTONS] = {
  PIN_A0,
  PIN_A1,
  PIN_A2,
  PIN_A3,
  PIN_A4,
  PIN_A5,
};

const char *button_names[NUM_BUTTONS][2] = {
  { "light", "toggle" },
  { "fan", "off" },
  { "fan", "low" },
  { "fan", "med" },
  { "fan", "high" },
  { "fan", "rev" }
};

const int fanId = 0;
const int PIN_LED = 3;

elapsedMillis toggle_timer; // switch debounce timer
elapsedMillis led_timer; // LED on time after button press
elapsedMillis breath_timer; // LED breath effect
byte breath_phase = 0;

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

  if (strcmp(attr, "light") == 0) { // Handle "Fan Light" commands
    if (strcmp(payload, "toggle") == 0) {
      command = 0b111110;
    }
  }
  else if (strcmp(attr, "fan") == 0) { // Handle "Fan Light" commands
    if (strcmp(payload, "off") == 0) {
      command = 0b111101;
    }
    else if (strcmp(payload, "low") == 0) {
      command = 0b110111;
    }
    else if (strcmp(payload, "med") == 0) {
      command = 0b101111;
    }
    else if (strcmp(payload, "high") == 0) {
      command = 0b011111;
    }
    else if (strcmp(payload, "rev") == 0) {
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

  for (int i = 0; i < NUM_BUTTONS; i++) {
    button[i].attach( button_pins[i], INPUT_PULLUP );
    button[i].interval(5);
    button[i].setPressedState(LOW);
  }

  ELECHOUSE_cc1101.Init();
  ELECHOUSE_cc1101.setMHZ(RF_FREQUENCY);
  ELECHOUSE_cc1101.SetTx();
  ELECHOUSE_cc1101.SpiStrobe(CC1101_SFSTXON);                 // standby

  mySwitch.setRepeatTransmit(RF_REPEATS);   // set RF code repeat
  mySwitch.setProtocol(RF_PROTOCOL);        // send Received Protocol
  mySwitch.setPulseLength(RF_PULSE_LENGTH); // modify this if required

  pinMode(PIN_LED, OUTPUT);
  analogWrite(PIN_LED, 0);
}

void loop() {
  //cc1101_debug.debug ();

  for (int i = 0; i < NUM_BUTTONS; i ++) {
    button[i].update();
    if (button[i].pressed() && toggle_timer >= 500) {
      toggle_timer = 0;
      analogWrite(PIN_LED, 255); // LED ON
      led_timer = 0;
      Serial.print(button_names[i][0]);
      Serial.print(" ");
      Serial.println(button_names[i][1]);
      transmitState(fanId, button_names[i][0], button_names[i][1]);
    }
  }

  if (led_timer >= 500) {
    led_timer = 500; // stay off
    if (breath_timer >= 11) {
      const float scale = .5;
      const int offset = 48;
      analogWrite(PIN_LED, gamma8(constrain(((int)sin8(breath_phase)-128)*scale+offset, 0, 255)));

      breath_timer = 0;
      if (breath_phase ++ == 180) {
        breath_phase = 0;
      }
    }
  }
}
