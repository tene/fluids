#!/usr/bin/perl
use strict;
use warnings;
use v5.12;
use feature ':5.12';
use lib qw/lib/;

use LB::Grid;

my $x = LB::Grid->new(2,9,8,8);
say $x->dump;
