#!/usr/bin/env perl

use strict 'vars';

my $addr = 0xF800;

open F, "<$ARGV[0]";
while (1) {
    my $d;
    my $len = read(F,$d,32);
    last if $len == 0;
    my @d = unpack('C*',$d);

    my $sum = $len + 3;
    $sum += ($addr >> 8);
    $sum += ($addr & 0xff);

    printf("S1%02x%04x", $len + 3, $addr);
    for (my $i = 0; $i < $len; $i++) {
        printf("%02x", $d[$i]);
        $sum += $d[$i];
    }
    printf("%x\n", ~$sum & 0xff);
    $addr += $len;
}
printf("S9\n");

close(F);
exit(0);

    

    

    
