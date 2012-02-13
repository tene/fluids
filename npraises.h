#ifndef NPRAISES_H_INCLUDED
#define NPRAISES_H_INCLUDED

#include <stdint.h>
#include <term.h>

uint8_t rgb_f(float r, float g, float b);

uint8_t gray_f(float v);

void set_fg(uint8_t c);

void set_bg(uint8_t c);

void reset_colors();

void setup_screen();

#endif
