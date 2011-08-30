package LB::Grid;
use strict;
use warnings FATAL => 'all';
use v5.12;
use feature ':5.12';
use Carp qw/confess croak/;
use List::Util qw/sum/;
use List::MoreUtils qw/pairwise/;

my $DEBUG = $ENV{'DEBUG'};

sub debug {
    if ($DEBUG) {
        say STDERR @_;
    }
}

for my $name (qw/x y dimensions count weight e o omega filled emptied/) {
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

sub boundary {
    my ($x, $y, $mx, $my) = @_;
    return 1 if $x < 4 && $y > $my-5;
    return 1 if $x >= 6 && $y == $my-7;
    return 1 if $x == 0 or $x == $mx-1 or $y == 0 or $y == $my-1;
    return 1 if ($x == 6) and $y < $my - 3 and $y >= $my-7;
    return 1 if $x == 9 and $y > $my - 4;
}

sub liquid {
    my ($x, $y, $mx, $my) = @_;
    return 1 if $x < 6;
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
        omega      => 0.85,
        filled     => [],
        emptied    => [],
    };
    bless $obj, $pkg;
    for my $x (0..$obj->x) {
        for my $y (0..$obj->y) {
            $obj->grid($x,$y)     = [(0) x $obj->count];
            $obj->buf($x,$y)     = [(0) x $obj->count];
            if (boundary($x, $y, $mx, $my)) {
                $obj->flags($x,$y) = 'boundary';
                $obj->mass($x,$y)  = 0;
            }
            elsif (liquid($x, $y, $mx, $my)) {
                $obj->flags($x,$y) = 'full';
                $obj->mass($x,$y)  = 1;
                $obj->grid($x,$y)  = $obj->eq([0,0.02], 1);
            }
            else {
                $obj->flags($x,$y) = 'empty';
                $obj->mass($x,$y)  = 0;
            }
        }
    }
    for my $x (0..$obj->x) {
        for my $y (0..$obj->y) {
            if ($obj->flags($x, $y) eq 'full' and grep { $obj->flags(@$_) ~~ ['empty']} @{$obj->neigh($x, $y)}) {
                $obj->flags($x,$y) = 'interface';
                $obj->mass($x,$y)  = 0.5;
                $obj->grid($x,$y)  = $obj->eq([0,0.02], 1);
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
                    $u->[1] += 0.01;
                    my $p = pressure($f);
                    my $eq = $self->eq($u, $p);
                    my $w = $self->omega;
                    my @new = pairwise { (1-$w) * $a + $w * $b } @$f, @$eq;
                    $self->buf($x,$y) = [@new];
                    if ($self->flags($x, $y) eq 'interface') {
                        my $m = $self->mass($x, $y);
                        if ($m > 1.001) {
                            debug "Noticed $x,$y is full";
                            push @{$self->filled}, [$x, $y];
                        }
                        elsif ($m < -0.001) {
                            debug "Noticed $x,$y is empty";
                            push @{$self->emptied}, [$x, $y];
                        }
                    }
                }
            }
        }
    }
    $self->swap;
    for (@{$self->filled}) {
        my ($x, $y) = @$_;
        die "Marked a non-interface cell as filled?  wtf?" unless $self->flags($x, $y) eq 'interface';
        debug("Filled $x,$y");
        for (@{$self->neigh($x, $y)}) {
            my ($nx, $ny) = @$_;
            given ($self->flags($nx, $ny)) {
                when ('empty') {
                    debug("Converting $nx, $ny from empty to interface");
                    $self->emptytointerface($nx, $ny);
                }
                when ('interface') {
                    $self->emptied = [ grep { not($_->[0] == $nx && $_->[1] == $ny) } @{$self->emptied}];
                }
            }
        }
        $self->flags($x, $y) = 'full';
    }

    for (@{$self->emptied}) {
        my ($x, $y) = @$_;
        die "Marked a non-interface cell as emptied?  wtf?" unless $self->flags($x, $y) eq 'interface';
        debug("Emptied $x,$y");
        for (@{$self->neigh($x, $y)}) {
            my ($nx, $ny) = @$_;
            given ($self->flags($nx, $ny)) {
                when ('full') {
                    debug("Converting $nx, $ny from full to interface");
                    $self->fulltointerface($nx, $ny);
                }
            }
        }
        $self->flags($x, $y) = 'empty';
    }
    for (@{$self->filled}, @{$self->emptied}) {
        $self->distributemass(@$_);
    }

    $self->filled = [];
    $self->emptied = [];
}

sub distributemass {
    my ($self, $x, $y) = @_;
    my @neigh =  @{$self->neigh($x, $y)};
    @neigh = grep { $self->flags(@$_) eq 'interface' } @neigh;
    my $count = @neigh;
    if ($count == 0) {
        debug( "Could not distribute mass from $x,$y; no interface neighbors");
    }
    my $m;
    my $normal = $self->normal($x, $y);
    given ($self->flags($x, $y)) {
        when ('full') {
            $m = $self->mass($x, $y) - pressure($self->grid($x, $y));
            $self->mass($x, $y) -= $m;
        }
        when ('empty') {
            $m = $self->mass($x, $y);
            $self->mass($x, $y) = 0;
        }
        default {
            confess "distributing mass from interface or boundary cell?  wtf?";
        }
    }
    my $total;
    for (@neigh) {
        my $dot = dot($normal, [$x-$_->[0], $y-$_->[1]]);
        $dot *= -1 if $self->flags($x, $y) eq 'empty';
        my $val = $dot > 0 ? $dot : 0;
        $total += $val;
        push @$_, $val;
    }
    for (@neigh) {
        my $val = pop @$_;
        if ($total > 0) {
            $self->mass(@$_) += $m * $val / $total;
        }
        else {
            $self->mass(@$_) += $m/$count;
        }
    }
}

sub emptytointerface {
    my ($self, $x, $y) = @_;
    die "Tried to initempty a nonempty cell" unless $self->flags($x, $y) eq 'empty';
    my $p = 0;
    my @u = ( 0,0 );
    my $count = 0;
    for (@{$self->neigh($x, $y)}) {
        my ($nx, $ny) = @$_;
        given ($self->flags($nx, $ny)) {
            when (['interface', 'filled']) {
                next if grep { $_->[0] == $nx && $_->[1] == $ny } @{$self->emptied};
                $p += pressure($self->grid($nx, $ny));
                my $nu = $self->u($self->grid($nx, $ny));
                for (0..($self->dimensions-1)) {
                    $u[$_] += $nu->[$_];
                }
                $count++;
            }
        }
    }
    if ($count > 0) {
        $p /= $count;
        @u = map { $_ / $count } @u;
        $self->grid($x, $y) = $self->eq([@u], $p);
        $self->flags($x, $y) = 'interface';
        $self->mass($x, $y) = 0;
    }
    else {
        #croak "Initializing an empty cell with no filled or interface neighbors?  wtf?";
        debug( "Could not convert $x,$y to interface; no interface neighbors" );
    }
}

sub fulltointerface {
    my ($self, $x, $y) = @_;
    die "Tried to initempty a nonfull cell" unless $self->flags($x, $y) eq 'full';
    $self->flags($x, $y) = 'interface';
}

sub neigh {
    my ($self, $x, $y) = @_;
    my @coords = map { [$x + $_->[0], $y + $_->[1]] } @{$self->e};
    shift @coords;
    return [@coords];
}

sub neighflagcount {
    my ($self, $x, $y, $flag) = @_;
    return 0 + map { $self->flags(@$_) eq $flag } @{$self->neigh($x, $y)};
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
    my $normal;
    # empty neighbors and full neighbors
    my ($neighempty, $neighfull);
    if ($flags eq 'interface') {
        $normal = $self->normal($x, $y);
        ($neighempty, $neighfull) = map { $self->neighflagcount($x, $y, $_) > 0 } qw/empty full/;
    }
    for my $i (0..($self->count-1)) {
        my ($dx, $dy) = @{$e[$i]};
        my $o = $o[$i];
        given ($self->flags($x+$dx, $y+$dy)) {
            when ('boundary') {
                $fp[$o] = $f->[$i];
            }
            when (['full', 'interface']) {
                $fp[$o] = $self->in($x, $y, $i);
                if ($flags eq 'interface') {
                    my $mul = 1;
                    my $dm = 0;
                    if ($_ eq 'interface' ) {
                        $mul = ($self->ff($x, $y) + $self->ff($x+$dx, $y+$dy))/2;
                        my ($ineighempty, $ineighfull) = map { $self->neighflagcount($x+$dx, $y+$dy, $_) > 0 } qw/empty full/;
                        my ($tmp, $itmp) = ("$neighempty,$neighfull", "$ineighempty,$ineighfull");
                        if ($tmp eq $itmp or !$ineighfull or !$neighempty) {
                            $dm += $fp[$o];
                        }
                        if ($tmp eq $itmp or !$ineighempty or !$neighfull) {
                            $dm -= $f->[$i];
                        }
                    }
                    else {
                        $dm = ($fp[$o] - $f->[$i]);
                    }
                    $mass += $mul * $dm;
                    if (dot($normal, $self->e->[$o]) > 0) {
                        $u //= $self->u($f);
                        $eq //= $self->eq($u, 1);
                        $fp[$o] = $eq->[$i] + $eq->[$o] - $f->[$i];
                    }
                }
                else {
                    $mass += ($fp[$o] - $f->[$i]);
                }
            }
            when ('empty') {
                $u //= $self->u($f);
                $eq //= $self->eq($u, 1);
                $fp[$o] = $eq->[$i] + $eq->[$o] - $f->[$i];
            }
            default {
                confess "wtf?  state $_ at " . $x+$dx . ", " . $y+$dy;
            }
        }
    }
    return [@fp], $mass;
}

sub ff {
    my ($self, $x, $y) = @_;
    given ($self->flags($x, $y)) {
        when ('interface') {
            return $self->mass($x, $y);
        }
        when ('full') {
            return $self->mass($x, $y) / pressure($self->grid($x, $y));
        }
        default {
            return 0;
        }
    }
}

sub normal {
    my ($self, $x, $y) = @_;
    my $nx = ($self->ff($x-1, $y) - $self->ff($x+1, $y))/2;
    my $ny = ($self->ff($x, $y-1) - $self->ff($x, $y+1))/2;
    return [$nx,$ny];
}

sub swap {
    my ($self) = @_;
    my $tmp = $self->{'grid'};
    $self->{'grid'} = $self->{'buf'};
    $self->{'buf'} = $tmp;
}

sub dump {
    #my @blocks = (' ', qw/▁ ▂ ▃ ▄ ▅ ▆ ▇ █/);
    my @shades = (232..255);
    my $self = shift;
    my @dump;
    my $mass = 0;
    for my $y (0..$self->y) {
        my @line;
        for my $x (0..$self->x) {
            $mass += $self->mass($x, $y);
            given ($self->flags($x,$y)) {
                when ('boundary') {
                    #$dump .= '▒▒';
                    push @line, 'boundary';
                }
                when ('empty') {
                    #$dump .= '  ';
                    push @line, 'empty';
                }
                default {
                    #my $m = pressure($self->grid($x,$y));
                    my $m = $self->mass($x, $y);
                    #$dump .= $blocks[int($m * (@blocks-1))] // do { say "ρ($x,$y)=$m"; '?'};
                    #$m = $m ** 2;
                    #my $shade = $shades[int($m*(@shades-1))] // $shades[-1];
                    #$line .= "\e[38;5;${shade}m██\e[0m";
                    push @line, $m;
                }
            }
        }
        push @dump, [@line];
    }
    #$dump .= "mass=$mass\n";
    #my ($u, $p, $m, $f) = ($self->u($self->grid(1,2)), pressure($self->grid(1,2)), $self->mass(1,2), $self->flags(1,2));
    #my ($x, $y) = @$u;
    #$dump .= "1,2 = $f u($x,$y) ρ($p) m($m)\n";
    return @dump;
}

1;

