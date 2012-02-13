#include <npraises.h>
#include <termios.h>
#define _BSD_SOURCE
#include <unistd.h>
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
    putp(tiparm(set_a_foreground, c));
}

void set_bg(uint8_t c) {
    putp(tiparm(set_a_background, c));
}

struct termios term_settings;
int setup_screen() {
    int rv;
    struct termios my_settings;
    rv = tcgetattr (0, &my_settings);
    if (rv < 0) {
        perror ("error in tcgetattr");
        return 0;
    }
    term_settings = my_settings;
    cfmakeraw(&my_settings);
    rv = tcsetattr (0, TCSANOW, &my_settings);
    if (rv < 0) {
        perror ("error in tcsetattr");
        return 0;
    }
    setupterm( "xterm-256color", 1, (int*)0 );
    putp(clear_screen);
    putp(enter_ca_mode);
    putp(orig_pair);
    putp(cursor_invisible);
    return 1;
}

void cleanup_screen() {
    putp(orig_pair);
    putp(exit_ca_mode);
    putp(cursor_normal);
    tcsetattr (0, TCSANOW, &term_settings);
}

void curs_xy(uint8_t x, uint8_t y) {
    putp(tiparm(tigetstr("cup"), y, x));
}
