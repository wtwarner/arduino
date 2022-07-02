#!/usr/bin/env perl
while (<>) {
    chomp;
    m/([0-9a-fA-F]+):\s*(.*)/;
    my @b = split /\s+/,$2;
    print pack("C[16]",map(hex, @b));
}
