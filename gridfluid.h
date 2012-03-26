#ifndef GRIDFLUID_H_INCLUDED
#define GRIDFLUID_H_INCLUDED

#include <stdint.h>
#include <stddef.h>

#pragma GCC visibility push(default)

typedef enum e_gridfluid_state {
    GF_OBSTACLE,
    GF_EMPTY,
    GF_FLUID,
    GF_INTERFACE
} gridfluid_state;

typedef struct gridfluid *gridfluid_t;

typedef struct gridfluid_properties {
    size_t x;
    size_t y;
    float gravity;
    float *pressure;
    float max_pressure;
    float min_pressure;
    float max_velocity;
} gridfluid_properties_t;

gridfluid_t gridfluid_create_empty_scene(size_t x, size_t y);

void gridfluid_free(gridfluid_t gf);

void gridfluid_set_obstacle(gridfluid_t gf, size_t x, size_t y);
void gridfluid_set_fluid(gridfluid_t gf, size_t x, size_t y);
void gridfluid_set_empty(gridfluid_t gf, size_t x, size_t y);
void gridfluid_set_gravity(gridfluid_t gf, float g);
uint8_t gridfluid_get_type(gridfluid_t gf, size_t x, size_t y);
gridfluid_properties_t *gridfluid_get_properties(gridfluid_t gf);

void gridfluid_step(gridfluid_t gf);

#pragma GCC visibility pop

#endif
