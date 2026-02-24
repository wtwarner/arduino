To assemble to binary:
  dasm therm.asm -f3 -otherm.bin -ltherm.lst
To S19:
  dasm therm.asm -f1 -otherm.s19  -ltherm.lst -stherm.sym

Original ROM image exp-10.bin dumped from 2716 ROM.
Disable original ROM image (exp-10.bin):
 ~/git/f9dasm/f9dasm -offset 0xf800 -6803 ~/Documents/Arduino/eprom-explorer-clock/exp-10.bin
