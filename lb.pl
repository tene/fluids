#!/usr/bin/perl
use strict;
use warnings;
use v5.12;
use feature ':5.12';
use lib qw/lib/;
$ENV{'TERM'}='xterm-256color';
use Curses;

use LB::Grid;
use Carp qw/croak confess/;

local $SIG{__DIE__} = sub {
    endwin;
    confess "Uncaught exception: @_" unless $^S;
};

my @shades = (232..255);
my $shades = @shades - 1;

my $win = Curses->new;
start_color;
noecho;
curs_set 0;

my $colors = COLORS;
my $cpcount = 1;
my %cache;
sub cp {
    my ($fg,$bg) = @_,0;
    return $cache{"$fg.$bg"} ||= $cpcount++;
}
for my $fg (@shades) {
    init_pair(cp($fg,$shades[0]), $fg, $shades[0]);
}


sub draw {
    my ($x,$y,$val) = @_;
    my $ch = '  ';
    my $fg = $shades[0];
    given ($val) {
        when ('boundary') {
            $fg = $shades[-1];
            $ch = '▒▒';
        }
        when ('empty') {
            $ch = '  ';
        }
        default {
            $fg = $shades[int($_*$shades)] // $shades[-1];
            $ch = '██';
        }
    }
    attron(COLOR_PAIR(cp($fg,$shades[0])));
    addstr($y,$x*2,$ch);
    attroff(COLOR_PAIR(cp($fg,$shades[0])));
}

my $count = $ARGV[0] // 5000;
my $drawstep = $ARGV[1] // 1;

my $lb = LB::Grid->new(2,9,20,25);

sub render {
    my (@grid) = @_;
    my ($x,$y) = (0,0);
    for my $line (@grid) {
        $x=0;
        for my $ch (@$line) {
            draw($x,$y,$ch);
            $x++;
        }
        $y++;
    }
}

for (1..$count) {
    spout() if $_ < 300;
    $lb->stream;
    if (($_ % $drawstep) == 0) {
        clear;
        render($lb->dump);
        doupdate();
        refresh;
    }
}

sub spout {
    for my $x (2..4) {
        for my $y (2..4) {
            $lb->grid($x,$y) = $lb->eq([0.2,0], 1);
            $lb->mass($x,$y) = 1;
            $lb->flags($x,$y) = 'interface' if $lb->flags($x,$y) eq 'empty';
        }
    }
     $lb->grid(3,3) = $lb->eq([0.30,0], 1);
     $lb->mass(3,3) = 1;
    $lb->flags(3,3) = 'full';
}

endwin;
