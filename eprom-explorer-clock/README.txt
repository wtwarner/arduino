To assemble to binary and make S19:
  dasm therm.asm -f3 -otherm.bin -ltherm.lst -stherm.sym
  ./mks19.pl therm.bin > therm.s19

Simulate:
 ~/sim68xx-0.9.9/src/boards/sim6301 therm.s19

Original ROM image exp-10.bin dumped from 2716 ROM.
Disable original ROM image (exp-10.bin):
 ~/git/f9dasm/f9dasm -offset 0xf800 -6803 ~/Documents/Arduino/eprom-explorer-clock/exp-10.bin
