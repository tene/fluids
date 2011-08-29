package LB::Grid;
use strict;
use warnings;
use v5.12;
use feature ':5.12';
use Carp qw/confess croak/;
use List::Util qw/sum/;
use List::MoreUtils qw/pairwise/;

for my $name (qw/x y dimensions count weight e o omega/) {
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

sub m {
    my ($self, $x, $y) = $_;
    if ($self->flags($x, $y) eq 'interface') {
        return $self->mass($x, $y);
    }
    else {
        return $self->pressure($x, $y);
    }
}

sub u {
    confess "Accessor 'u' requires weight list argument" unless @_ == 2;
    my ($self, $weights) = @_;
    my @u = ((0) x $self->dimensions);
    for (0..($self->count-1)) {
        my $f = $weights->[$_];
        my @e = @{$self->e->[$_]};
        @u = map { $u[$_] + $f * $e[$_] } 0..1;
    }
    return [@u];
}

sub eq {
    confess "Accessor 'eq' requires vector and pressure arguments" if @_ < 3;
    my ($self, $u, $p) = @_;
    my ($x,$y) = @$u;
    my $usqr = lensqr($u);
    # eq[i] = w[i] * ( p + 3*dot(e[i], u) - (3/2)*usqr + (9/2)*(dot(e[i], u)**2) )
    my @eq = pairwise {
        my ($dir, $w) = ($a, $b);
        my $edu = dot($dir, $u);
        $w * ( $p + 3*$edu - 1.5*$usqr + 4.5*($edu**2) );
    } @{$self->e}, @{$self->weight};
    return [@eq];
}

sub pressure {
    my ($list) = @_;
    return sum @$list;
}

sub lensqr {
    my ($v) = @_;
    sum map { $_ ** 2 } @$v;
}

sub dot {
    my ($i, $j) = @_;
    sum pairwise {$a * $b} @$i, @$j;
}

# 012345678
# ·→↓←↑↘↙↖↗
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
        omega      => 0.5,
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
            elsif ($x == $y or $x-1 == $y) {
                $obj->flags($x,$y) = 'interface';
                $obj->mass($x,$y)  = 0.5;
                $obj->grid($x,$y)  = $obj->eq([0,0.02], 1);
            }
            elsif ($x < $y) {
                $obj->flags($x,$y) = 'fluid';
                $obj->mass($x,$y)  = 1;
                $obj->grid($x,$y)  = $obj->eq([0,0.02], 1);
            }
            else {
                confess "wtf";
            }
        }
    }
    return $obj;
}

sub stream {
    my ($self) = @_;
    for my $y (0..$self->y) {
        for my $x (0..$self->x) {
            given ($self->flags($x,$y)) {
                when (['boundary', 'empty']) {
                    $self->buf($x,$y) = $self->grid($x,$y);
                }
                default {
                    my ($f, $m)  = $self->gather($x,$y);
                    $self->mass($x, $y) += $m;
                    my $u = $self->u($f);
                    my $p = pressure($f);
                    my $eq = $self->eq($u, $p);
                    my $w = $self->omega;
                    my @new = pairwise { (1-$w) * $a + $w * $b } @$f, @$eq;
                    $self->buf($x,$y) = [@new];
                }
            }
        }
    }
    $self->swap;
}

sub in {
    my ($self, $x, $y, $i) = @_;
    my ($dx, $dy) = @{$self->e->[$i]};
    $self->grid($x+$dx, $y+$dy)->[$self->o->[$i]];
}

sub out {
    my ($self, $x, $y, $i) = @_;
    $self->grid($x, $y)->[$i];
}

sub gather {
    my ($self, $x, $y) = @_;
    my @e = @{$self->e};
    my @o = @{$self->o};
    my $f = $self->grid($x, $y);
    my $flags = $self->flags($x, $y);

    # return list;
    my @fp;

    # used for interface cells
    my $u;
    my $eq;

    my $mass = 0;
    for my $i (0..($self->count-1)) {
        my ($dx, $dy) = @{$e[$i]};
        my $o = $o[$i];
        given ($self->flags($x+$dx, $y+$dy)) {
            when ('boundary') {
                $fp[$o] = $f->[$i];
            }
            when (['fluid', 'interface']) {
                $fp[$o] = $self->in($x, $y, $i);
                my $mul = 1;
                if ($_ eq 'interface' and $flags eq 'interface') {
                    $mul = ($self->ff($x, $y) + $self->ff($x+$dx, $y+$dy))/2;
                }
                $mass += $mul * ($fp[$o] - $f->[$i]);
            }
            when ('empty') {
                $u //= $self->u($f);
                $eq //= $self->eq($u, 1);
                $fp[$o] = $eq->[$i] + $eq->[$o] - $f->[$i];
            }
        }
    }
    return [@fp], $mass;
}

sub ff {
    my ($self, $x, $y) = @_;
    if ($self->flags($x, $y) eq 'interface') {
        return $self->mass($x, $y);
    }
    else {
        return $self->mass($x, $y) / pressure($self->grid($x, $y));
    };
}

sub swap {
    my ($self) = @_;
    my $tmp = $self->{'grid'};
    $self->{'grid'} = $self->{'buf'};
    $self->{'buf'} = $tmp;
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
                    my $m = pressure($self->grid($x,$y));
                    $dump .= $blocks[int($m * (@blocks-1))] // do { say "ρ($x,$y)=$m"; '?'};
                }
            }
        }
        $dump .= "\n";
    }
    return $dump;
}

1;
