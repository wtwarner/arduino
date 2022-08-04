# arduino
This contains several of my Arduino-based projects:

STM32-GPSO: clone of AndrewBCN/STM32-GPSDO with a couple minor changes

eeprom-programmer: based on Ben Eater's, modified for type 2716 EEPROM with Missile Command arcade ROMs

iv25-clock: analog-style clock using 24 IV25 VFD tubes, DS3231 real-time clock, and BlueTooth serial for configuration.  SparkFun Pro Micro based.

metronome: Nixie-tube display, using Teensy 4.0 with Audio Adapter and Adafruit TPA2016 audio amp board.  

nixie: Nixie-tube clock, with DS3231 real-time clock, and ArduinoBLE Bluetooth control, using Arduino Nano 33 IoT

shift_bt: Neopixel LEDs in epoxy river table, Teensy 2.0 with Bluetooth Serial control.

teleregister:  Teleregister-based clock with DS3231 real-time clock and IR control.  Arduino Nano based.  
     See Fran Blanche's viddeo at https://www.youtube.com/watch?v=c6l2vbn6IXM&t=929s for info on the Teleregister. 
     
tuner: guitar tuner using IN-9 bar graph tubes, VFD alphanumeric displays, Eye tube. Teensy 3.2 based.  Uses NoteFreq from Teensy audio library, 
     and "Yin" frequency detection library.
