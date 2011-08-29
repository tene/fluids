#!/usr/bin/perl
use strict;
use warnings;
use v5.12;
use feature ':5.12';
use lib qw/lib/;

use LB::Grid;
use Carp qw/croak confess/;

local $SIG{__DIE__} = sub {
    confess "Uncaught exception: @_" unless $^S;
};

my $count = $ARGV[0] // 10;

my $x = LB::Grid->new(2,9,14,16);
say $x->dump;
for (1..$count) {
    $x->stream;
    say $x->dump if (($_ % 10) == 0);
}
