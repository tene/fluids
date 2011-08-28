package LB::Grid;
use strict;
use warnings;
use v5.12;
use feature ':5.12';
use ShittyObject qw/attr/;
use List::Util qw/sum/;
use List::MoreUtils qw//;

attr($_) for qw/x y u v dimensions count/;

sub new {
    my $pkg = shift;
    my ($dim, $count, $x, $y) = @_;
    my $obj = {
        x => $x - 1,
        y => $y - 1,
        dimensions => $dim,
        count => $count,
    };
    for my $a (qw/u v/) {
        my @dim;
        for my $i (1..$y) {
            my @i;
            for my $j (1..$x) {
                my @j;
                for my $k (1..$count) {
                    push @j, 0;
                }
                push @i, [@j];
            }
            push @dim, [@i];
        }
        $obj->{$a} = [@dim];
    }
    bless $obj, $pkg;
    return $obj;
}

sub dump {
    my $self = shift;
    my $dump = '';
    for my $x (0..$self->x) {
        for my $y (0..$self->y) {
            $dump .= sum(@{$self->u->[$x][$y]});
        }
        $dump .= "\n";
    }
    return $dump;
}

1;
