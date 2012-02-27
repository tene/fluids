#ifndef GRIDFLUID_H_INCLUDED
#define GRIDFLUID_H_INCLUDED

#include <stdint.h>

#pragma GCC visibility push(default)

typedef enum e_gridfluid_state {
    GF_OBSTACLE,
    GF_EMPTY,
    GF_FLUID,
    GF_INTERFACE
} gridfluid_state;

typedef struct gridfluid *gridfluid_t;

gridfluid_t gridfluid_create_empty_scene(uint8_t x, uint8_t y);

void gridfluid_free(gridfluid_t gf);

void gridfluid_set_obstacle(gridfluid_t gf, uint8_t x, uint8_t y);
void gridfluid_set_fluid(gridfluid_t gf, uint8_t x, uint8_t y);
void gridfluid_set_empty(gridfluid_t gf, uint8_t x, uint8_t y);
void gridfluid_set_gravity(gridfluid_t gf, float g);
uint8_t gridfluid_get_type(gridfluid_t gf, uint8_t x, uint8_t y);

void gridfluid_step(gridfluid_t gf);

#pragma GCC visibility pop

#endif
