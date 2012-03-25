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
    float *pressure;
    float max_pressure;
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

float omega=0.5;

static void gridfluid_stream(gridfluid_t gf) {
    for (size_t x = 0; x < gf->x; x++) {
        for (size_t y = 0; y < gf->y; y++) {
            gridfluid_cell_t *source = &GF_CELL(gf,x,y);
            gridfluid_cell_t *dest = &GF_NEXTCELL(gf,x,y);
            if (source->flags != GF_FLUID) {
                *dest = *source;
                continue;
            }
            dest->flags = source->flags;
            for (size_t i=0; i<9; i++) {
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

static void gridfluid_cell_macro(float *df, float *pressure, float *ux, float *uy) {
    *pressure = 0;
    *ux = 0;
    *uy = 0;
    for (size_t i=0; i<9; i++) {
        float f = df[i];
        *ux += f*velocities[i][0];
        *uy += f*velocities[i][1];
        *pressure += f;
    }
}

static void gridfluid_eq(float pressure, float ux, float uy, float *eq) {
    float usqr = ux*ux + uy*uy;
    // w_i(ρ + 3e_i · u − (3/2)*u^2 + (9/2)*(ei · u)^2)
    for (size_t i=0; i<9; i++) {
        float w = weights[i];
        float ex = velocities[i][0];
        float ey = velocities[i][1];
        float edotu = (ex*ux + ey*uy);
        eq[i] = w * ( pressure + 3*edotu - 1.5*usqr + 4.5*edotu*edotu);
        if (eq[i] < 0)
            abort();
    }
}

static void gridfluid_collide(gridfluid_t gf) {
    float pressure=0;
    float ux=0;
    float uy=0;
    float eq[9];
    float max_pressure = 0;
    for (size_t i=0; i < gf->x * gf->y; i++) {
        gridfluid_cell_t *cell = &gf->grid[i];
        if (cell->flags != GF_FLUID) {
            continue;
        }
        gridfluid_cell_macro(cell->df, &pressure, &ux, &uy);
        if (pressure > 100000)
            abort();
        uy -= 0.009;
        gridfluid_eq(pressure, ux, uy, eq);
        gf->pressure[i] = pressure;
        if (pressure > max_pressure)
            max_pressure = pressure;
        for (size_t j=0; j<9; j++) {
            float f = cell->df[j];
            //float nf = f - omega * (f - eq[j]);
            float nf = f * (1-omega) + omega * eq[j];
            if (nf < 0)
                abort();
            cell->df[j] = nf;
        }
    }
    gf->max_pressure = max_pressure;
}


gridfluid_t gridfluid_create_empty_scene(size_t x, size_t y) {
    gridfluid_t gf = calloc(1, sizeof(struct gridfluid));
    gf->x = x;
    gf->y = y;
    gf->grid = calloc(x*y, sizeof(gridfluid_cell_t));
    gf->nextgrid = calloc(x*y, sizeof(gridfluid_cell_t));
    gf->pressure = calloc(x*y, sizeof(float));
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
    gridfluid_eq(1,0.5,0.00,cell->df);
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

float gridfluid_get_pressure(gridfluid_t gf, size_t x, size_t y) {
    return( gf->pressure[ x + y * gf->x ] );
}

float gridfluid_get_max_pressure(gridfluid_t gf) {
    return( gf->max_pressure );
}

void gridfluid_step(gridfluid_t gf) {
    gridfluid_stream(gf);
    gridfluid_cell_t *tmp = gf->grid;
    gf->grid = gf->nextgrid;
    gf->nextgrid = tmp;
    gridfluid_collide(gf);
}
