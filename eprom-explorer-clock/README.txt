Two things here:
1. EEPROM burning program
2. HP 1984-85 Explorer program clock/thermometer code

Two versions of clock/thermometer code here:
1. original exp-10.bin read from EPROM
2. slightly modified therm.bin assembled from therm.asm

1. Original ROM image exp-10.bin dumped from 2716 ROM.
Disable original ROM image (exp-10.bin):
 ~/git/f9dasm/f9dasm -offset 0xf800 -6803 exp-10.bin

2. To assemble to binary and make S19:
  dasm therm.asm -f3 -otherm.bin -ltherm.lst -stherm.sym
  ./mks19.pl therm.bin > therm.s19

  Simulate (not working yet)
    ~/sim68xx-0.9.9/src/boards/sim6301 therm.s19


*** Changes in therm.asm relative to original:
1. default alternate display mode 0x6 = F + H (time)
2. default year 2026
3. daylight savings start 2nd week March, end 1st week November

Convert the .bin to C source for the burning program:
 xxd -i therm.bin > therm.ino
 xxd -i exp-10.bin > exp-10.ino
change array type to 'PROGMEM const'
