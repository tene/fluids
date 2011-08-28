package ShittyObject;
use strict;
use warnings;
use v5.12;
use feature ':5.12';
use parent 'Exporter';
our @EXPORT_OK = qw/attr/;

sub attr {
    my ($name) = @_;
    my ($caller) = caller;
    no strict 'refs';
    *{"${caller}::$name"} = sub :lvalue {
        my ($self, $val) = @_;

        if (defined($val)) {
            $self->{$name} = $val;
        }
        $self->{$name};
    }
}

1;
