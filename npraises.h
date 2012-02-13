#ifndef NPRAISES_H_INCLUDED
#define NPRAISES_H_INCLUDED

#define _BSD_SOURCE
#include <unistd.h>
#include <stdint.h>
#include <term.h>

uint8_t rgb_f(float r, float g, float b);

uint8_t gray_f(float v);

void set_fg(uint8_t c);

void set_bg(uint8_t c);

int setup_screen();

void cleanup_screen();

void curs_xy(uint8_t x, uint8_t y);

#endif
