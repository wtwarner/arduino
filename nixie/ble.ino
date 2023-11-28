/*
  Callback LED

  This example creates a BLE peripheral with service that contains a
  characteristic to control an LED. The callback features of the
  library are used.

  The circuit:
  - Arduino MKR WiFi 1010, Arduino Uno WiFi Rev2 board, Arduino Nano 33 IoT,
    Arduino Nano 33 BLE, or Arduino Nano 33 BLE Sense board.

  You can use a generic BLE central app, like LightBlue (iOS and Android) or
  nRF Connect (Android), to interact with the services and characteristics
  created in this sketch.

  This example code is in the public domain.
*/

#include <ArduinoBLE.h>

BLEService nixieService("903b13ce-eadb-482f-b8fa-a5a2ecffffb4"); // create service
BLEStringCharacteristic sCharacteristic("2BDE", // FixedString64
                                        BLEWriteWithoutResponse|BLENotify, 64);
//BLEStringCharacteristic sRspCharacteristic("2A05", // Service Changed
//                                        BLERead, 64);
BLEDescriptor sDescriptor("2901", "Nixie command");

void ble_setup() {

  // begin initialization
  if (!BLE.begin()) {
    Serial.println("starting BLE failed!");

    return;
  }

  // set the local name peripheral advertises
  BLE.setDeviceName("Nixie");
  BLE.setLocalName("Nixie");
  // set the UUID for the service this peripheral advertises
  BLE.setAdvertisedService(nixieService);

  // add the characteristic to the service
  nixieService.addCharacteristic(sCharacteristic);
  //nixieService.addCharacteristic(sRspCharacteristic);

  // add service
  BLE.addService(nixieService);

  // set an initial value for the characteristic
  sCharacteristic.writeValue("----");

  // start advertising
  BLE.advertise();

  Serial.println("Bluetooth device active, waiting for connections...");
}

bool ble_loop(String &cmd) {
  // poll for BLE events
  static bool has_central = false;

  BLEDevice central = BLE.central();
    if (central && !has_central) {
      Serial.println("BT Connected");
    }
    if (!central && has_central) {
      Serial.println("BT Disconnected");
    }
 if (central && sCharacteristic.written()) {
    cmd = sCharacteristic.value();

    Serial.println("BT cmd: ");
    Serial.println(cmd);
    return true;
  }
  has_central = central;
  return false;
}

void ble_output(String &s)
{
  sCharacteristic.writeValue(s);
}
