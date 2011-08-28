package LB::Grid;
use strict;
use warnings;
use v5.12;
use feature ':5.12';
use Carp qw/confess croak/;
use List::Util qw/sum/;
use List::MoreUtils qw/pairwise/;

for my $name (qw/x y dimensions count weight e o/) {
    no strict 'refs';
    *{__PACKAGE__."::$name"} = sub :lvalue {
        $_[0]{$name};
    }
}

for my $name (qw/grid buf flags mass/) {
    no strict 'refs';
    *{__PACKAGE__."::$name"} = sub :lvalue {
        confess "Accessor '$name' requires x and y arguments" unless @_ == 3;
        $_[0]{$name}[$_[1]][$_[2]];
    }
}

sub pressure {
    confess "Accessor 'pressure' requires x and y arguments" unless @_ == 3;
    my ($self, $x, $y) = @_;
    return sum @{$self->grid($x, $y)};
}

sub u {
    confess "Accessor 'u' requires x and y arguments" unless @_ == 3;
    my ($self, $x, $y) = @_;
    my $g = $self->grid($x, $y);
    my @u = ((0) x $self->dimensions);
    for (0..($self->count-1)) {
        my $f = $g->[$_];
        my @e = @{$self->e->[$_]};
        @u = map { $u[$_] + $f * $e[$_] } 0..1;
    }
    return [@u];
}

sub eq {
    confess "Accessor 'eq' requires x and y arguments" unless @_ == 3;
    my ($self, $u, $p) = @_;
    my $usqr = lensqr $u;
    # eq[i] = w[i] * ( p + 3*dot(e[i], u) - (3/2)*usqr + (9/2)*(dot(e[i], u)**2) )
    my @eq = pairwise {
        my $edu = dot($a, $u);
        $b * ( $p + 3*$edu - 1.5*$usqr + 4.5*($edu**2));
    } @{$self->e}, @{$self->weight};
    return [@eq];
}

sub lensqr {
    my ($v) = @_;
    sum map { $_ ** 2 } @$v;
}

sub dot {
    my ($i, $j) = @_;
    sum pairwise {$a * $b} @$i, @$j;
}

sub new {
    my $pkg = shift;
    my ($dim, $count, $mx, $my) = @_;
    croak 'Models other than D2Q9 NYI' unless $dim == 2 and $count == 9;
    my $obj = {
        x          => $mx - 1,
        y          => $my - 1,
        dimensions => $dim,
        count      => $count,
        weight     => [ 4/9, 1/9, 1/9, 1/9, 1/9, 1/36, 1/36, 1/36, 1/36 ],
        e          => [ [ 0,0 ], [ 1,0 ], [ 0,1 ], [ -1,0 ], [ 0,-1 ], [ 1,1 ], [ -1,1 ], [ -1,-1 ], [ 1,-1 ] ],
        o          => [0, 3, 4, 1, 2, 7, 8, 5, 6],
    };
    bless $obj, $pkg;
    for my $x (0..$obj->x) {
        for my $y (0..$obj->y) {
            $obj->grid($x,$y)     = [(0) x $obj->count];
            $obj->buf($x,$y)     = [(0) x $obj->count];
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
