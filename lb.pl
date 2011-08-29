#!/usr/bin/perl
use strict;
use warnings;
use v5.12;
use feature ':5.12';
use lib qw/lib/;

use LB::Grid;
use Data::Dump;

my $count = $ARGV[0] // 10;

my $x = LB::Grid->new(2,9,16,8);
say $x->dump;
for (1..$count) {
    $x->stream;
    say $x->dump;
}
