#include "gridfluid.h"
#include <stdlib.h>

typedef struct gridfluid_cell {
    float df[9];
    gridfluid_state flags;
} gridfluid_cell_t;

struct gridfluid {
    size_t x;
    size_t y;
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

#define GF_CELL(gf,cx,cy) (gf->grid[(cx) + (cy)*gf->x])
#define GF_NEXTCELL(gf,cx,cy) (gf->nextgrid[(cx) + (cy)*gf->x])

void gridfluid_stream(gridfluid_t gf) {
    for (size_t x = 0; x < gf->x; x++) {
        for (size_t y = 0; y < gf->y; y++) {
            gridfluid_cell_t *source = &GF_CELL(gf,x,y);
            gridfluid_cell_t *dest = &GF_NEXTCELL(gf,x,y);
            if (source->flags != GF_FLUID) {
                *dest = *source;
                continue;
            }
            dest->flags = source->flags;
            for (size_t i=1; i<9; i++) {
                int8_t dx = velocities[i][0];
                int8_t dy = velocities[i][1];
                gridfluid_cell_t *neigh = &GF_CELL(gf,x+dx,y+dy);
                switch(neigh->flags) {
                    case GF_OBSTACLE:
                        dest->df[i] = source->df[rindex[i]];
                        break;
                    case GF_FLUID:
                        dest->df[i] = neigh->df[i];
                        break;
                    default:
                        break;
                }
            }
        }
    }
}

void gridfluid_collide(gridfluid_t gf) {
}

gridfluid_t gridfluid_create_empty_scene(size_t x, size_t y) {
    gridfluid_t gf = calloc(1, sizeof(struct gridfluid));
    gf->x = x;
    gf->y = y;
    gf->grid = calloc(x*y, sizeof(gridfluid_cell_t));
    gf->nextgrid = calloc(x*y, sizeof(gridfluid_cell_t));
    for (size_t i = 1; i < x-1; i++) {
        for (size_t j = 1; j < y-1; j++) {
            gridfluid_set_fluid(gf, i, j);
        }
    }
    return(gf);
}

void gridfluid_free(gridfluid_t gf) {
    free(gf->grid);
    free(gf);
}

void gridfluid_set_obstacle(gridfluid_t gf, size_t x, size_t y) {
    GF_CELL(gf,x,y).flags = GF_OBSTACLE;
}

void gridfluid_set_fluid(gridfluid_t gf, size_t x, size_t y) {
    gridfluid_cell_t *cell = &GF_CELL(gf,x,y);
    for (size_t i = 0; i<9; i++) {
        cell->df[i] = 1;
    }
    cell->flags = GF_FLUID;
}

void gridfluid_set_empty(gridfluid_t gf, size_t x, size_t y) {
    GF_CELL(gf,x,y).flags = GF_EMPTY;
}

void gridfluid_set_gravity(gridfluid_t gf, float g) {
    gf->gravity = g;
}

uint8_t gridfluid_get_type(gridfluid_t gf, size_t x, size_t y) {
    return( GF_CELL(gf,x,y).flags );
}

void gridfluid_step(gridfluid_t gf) {
    gridfluid_stream(gf);
}
