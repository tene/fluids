#include <npraises.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <term.h>

uint8_t rgb_f(float r, float g, float b) {
    uint8_t ri = r*6;
    uint8_t gi = g*6;
    uint8_t bi = b*6;
    ri = ri > 5 ? 5 : ri;
    gi = gi > 5 ? 5 : gi;
    bi = bi > 5 ? 5 : bi;
    return( 16 + 36*ri + 6*gi + bi );
}

uint8_t gray_f(float v) {
    int ansi = 26*v;
    ansi = ansi == 0 ?
        16 :
        ansi >= 25 ?
            15:
            ansi + 231;
    return( ansi );
}

void set_fg(uint8_t c) {
    printf("\e[38;5;%um", c);
}

void set_bg(uint8_t c) {
    printf("\e[48;5;%um", c);
}

void reset_colors() {
    printf("\e[0m");
}

void setup_screen() {
    setupterm( (char*)0, 1, (int*)0 );
    putp(clear_screen);
};
