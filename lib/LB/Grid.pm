package LB::Grid;
use strict;
use warnings;
use v5.12;
use feature ':5.12';
use Carp qw/confess/;
use List::Util qw/sum/;
use List::MoreUtils qw//;

for my $name (qw/x y dimensions count weight vel o/) {
    no strict 'refs';
    *{__PACKAGE__."::$name"} = sub :lvalue {
        $_[0]{$name};
    }
}

for my $name (qw/u v flags mass/) {
    no strict 'refs';
    *{__PACKAGE__."::$name"} = sub :lvalue {
        confess "Accessor $name requires x and y arguments" unless @_ == 3;
        $_[0]{$name}[$_[1]][$_[2]];
    }
}

sub new {
    my $pkg = shift;
    my ($dim, $count, $mx, $my) = @_;
    my $obj = {
        x          => $mx - 1,
        y          => $my - 1,
        dimensions => $dim,
        count      => $count,
        weight     => [ 4/9, 1/9, 1/9, 1/9, 1/9, 1/36, 1/36, 1/36, 1/36 ],
        vel        => [ [ 0,0 ], [ 1,0 ], [ 0,1 ], [ -1,0 ], [ 0,-1 ], [ 1,1 ], [ -1,1 ], [ -1,-1 ], [ 1,-1 ] ],
        o          => [0, 3, 4, 1, 2, 7, 8, 5, 6],
    };
    bless $obj, $pkg;
    for my $x (0..$obj->x) {
        for my $y (0..$obj->y) {
            $obj->u($x,$y)     = [(0) x $obj->count];
            $obj->v($x,$y)     = [(0) x $obj->count];
            if ($x == 0 or $x == $obj->x or $y == 0 or $y == $obj->y) {
                $obj->flags($x,$y) = 'boundary';
                $obj->mass($x,$y)  = 0;
            }
            elsif ($x > $y) {
                $obj->flags($x,$y) = 'empty';
                $obj->mass($x,$y)  = 0;
            }
            elsif ($x == $y) {
                $obj->flags($x,$y) = 'interface';
                $obj->mass($x,$y)  = 0.5;
            }
            elsif ($x < $y) {
                $obj->flags($x,$y) = 'fluid';
                $obj->mass($x,$y)  = 1;
            }
            else {
                confess "wtf";
            }
        }
    }
    return $obj;
}

sub dump {
    my @blocks = (' ', qw/▁ ▂ ▃ ▄ ▅ ▆ ▇ █/);
    my $self = shift;
    my $dump = '';
    for my $y (0..$self->y) {
        for my $x (0..$self->x) {
            given ($self->flags($x,$y)) {
                when ('boundary') {
                    $dump .= '▒';
                }
                default {
                    my $m = $self->mass($x,$y);
                    $dump .= $blocks[int($m * (@blocks-1))];
                }
            }
        }
        $dump .= "\n";
    }
    return $dump;
}

1;
