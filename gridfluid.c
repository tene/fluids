#include "gridfluid.h"
#include <stdlib.h>

typedef struct gridfluid_cell {
    float df[9];
    gridfluid_state flags;
} gridfluid_cell_t;

struct gridfluid {
    uint8_t x;
    uint8_t y;
    float gravity;
    gridfluid_cell_t *grid;
    gridfluid_cell_t *nextgrid;
};

static const float weights[9] = { 4./9., 1./9., 1./9., 1./9., 1./9.,
                             1./36., 1./36., 1./36., 1./36. };

static const float velocities[9][2] = {
    {0,0},
    {1,0}, {0,1}, {-1,0}, {0,-1},
    {1,1}, {-1,1}, {-1,-1}, {1,-1}
};

static const uint8_t rindex[9] = {
    0,3,4,1,2,7,8,5,6
};

#define GF_CELL(gf,cx,cy) (gf->grid[cx + cy*gf->x])

gridfluid_t gridfluid_create_empty_scene(uint8_t x, uint8_t y) {
    gridfluid_t gf = calloc(1, sizeof(struct gridfluid));
    gf->grid = calloc(x*y, sizeof(gridfluid_cell_t));
    gf->nextgrid = calloc(x*y, sizeof(gridfluid_cell_t));
    for (uint8_t i = 1; i < x-1; i++) {
        for (uint8_t j = 1; j < y-1; j++) {
            gridfluid_set_empty(gf, i, j);
        }
    }
    return(gf);
}

void gridfluid_free(gridfluid_t gf) {
    free(gf->grid);
    free(gf);
}

void gridfluid_set_boundary(gridfluid_t gf, uint8_t x, uint8_t y) {
    GF_CELL(gf,x,y).flags = GF_BOUNDARY;
}

void gridfluid_set_fluid(gridfluid_t gf, uint8_t x, uint8_t y) {
    // set DFs
    GF_CELL(gf,x,y).flags = GF_FLUID;
}

void gridfluid_set_empty(gridfluid_t gf, uint8_t x, uint8_t y) {
    GF_CELL(gf,x,y).flags = GF_EMPTY;
}

void gridfluid_set_gravity(gridfluid_t gf, float g) {
    gf->gravity = g;
}

uint8_t gridfluid_get_type(gridfluid_t gf, uint8_t x, uint8_t y) {
    return( GF_CELL(gf,x,y).flags );
}

void gridfluid_step(gridfluid_t gf) {
    abort();
}
