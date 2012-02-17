#ifndef GRIDFLUID_H_INCLUDED
#define GRIDFLUID_H_INCLUDED

#include <stdint.h>

typedef struct gridfluid {
    uint8_t x;
    uint8_t y;
    uint8_t *flags;
    float *field;
} *gridfluid_t;

gridfluid_t gridfluid_create_empty_scene(uint8_t x, uint8_t y);

void gridfluid_set_boundary(gridfluid_t gf, uint8_t x, uint8_t y);
void gridfluid_set_fluid(gridfluid_t gf, uint8_t x, uint8_t y);
void gridfluid_set_empty(gridfluid_t gf, uint8_t x, uint8_t y);
void gridfluid_set_gravity(gridfluid_t gf, float g);
uint8_t gridfluid_get_type(gridfluid_t gf, uint8_t x, uint8_t y);

#endif
