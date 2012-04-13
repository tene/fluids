#include "gridfluid.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

typedef struct gridfluid_cell {
    float df[9];
    gridfluid_state flags;
    float mass;
    float fluid;
} gridfluid_cell_t;

typedef enum e_gridfluid_change_flag {
    GF_CHANGE_NONE,
    GF_CHANGE_EMPTIED,
    GF_CHANGE_FILLED
} gridfluid_change_flag;

struct gridfluid {
    size_t x;
    size_t y;
    float gravity;
    float atmosphere;
    gridfluid_cell_t *grid;
    gridfluid_cell_t *nextgrid;
    gridfluid_properties_t *props;
    gridfluid_change_flag *changeflags;
    size_t filled;
    size_t emptied;
};

static const float change_fudge = 0.001;

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

float omega=0.75;

static void neighcount(gridfluid_t gf, size_t x, size_t y, size_t *empty, size_t *fluid) {
    *fluid = 0;
    *empty = 0;
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            if (dx == 0 && dx == dy)
                continue;
            switch(GF_CELL(gf, x+dx, y+dy).flags) {
                case GF_FLUID:
                    *fluid = *fluid+1;
                    break;
                case GF_EMPTY:
                    *empty = *empty+1;
                    break;
                default:
                    break;
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

static void gridfluid_cell_normal(gridfluid_t gf, size_t x, size_t y, float *ux, float *uy) {
    *ux = (GF_CELL(gf,x-1,y).fluid - GF_CELL(gf,x+1,y).fluid)/2;
    *uy = (GF_CELL(gf,x,y-1).fluid - GF_CELL(gf,x,y+1).fluid)/2;
}

static float dot2f(float ax, float ay, float bx, float by) {
    return ax*bx + ay*by;
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

static void gridfluid_stream(gridfluid_t gf) {
    for (size_t x = 0; x < gf->x; x++) {
        for (size_t y = 0; y < gf->y; y++) {
            gridfluid_cell_t *source = &GF_CELL(gf,x,y);
            gridfluid_cell_t *dest = &GF_NEXTCELL(gf,x,y);
            dest->flags = source->flags;
            dest->mass = source->mass;
            float pressure=0;
            size_t emptycount;
            size_t fluidcount;
            switch (source->flags) {
                case GF_FLUID:
                    for (size_t i=0; i<9; i++) {
                        size_t o = rindex[i];
                        int8_t dx = velocities[i][0];
                        int8_t dy = velocities[i][1];
                        gridfluid_cell_t *neigh = &GF_CELL(gf,x+dx,y+dy);
                        switch(neigh->flags) {
                            case GF_OBSTACLE:
                                dest->df[o] = source->df[i];
                                break;
                            case GF_FLUID:
                            case GF_INTERFACE:
                                dest->mass += neigh->df[o];
                                dest->mass -= source->df[i];
                                dest->df[o] = neigh->df[o];
                                break;
                            default:
                                break;
                        }
                        pressure += dest->df[o];
                        dest->fluid = 1;
                        //dest->mass = pressure;
                    }
                    break;
                case GF_INTERFACE:
                    neighcount(gf, x, y, &emptycount, &fluidcount);
                    float eqdf[9];
                    float oldpressure, ux, uy;
                    float nx, ny;
                    gridfluid_cell_normal(gf, x, y, &nx, &ny);
                    gridfluid_cell_macro(source->df, &oldpressure, &ux, &uy);
                    gridfluid_eq(gf->atmosphere, ux, uy, eqdf);
                    for (size_t i=0; i<9; i++) {
                        size_t o = rindex[i];
                        int8_t dx = velocities[i][0];
                        int8_t dy = velocities[i][1];
                        size_t iemptycount, ifluidcount;
                        gridfluid_cell_t *neigh = &GF_CELL(gf,x+dx,y+dy);
                        switch(neigh->flags) {
                            case GF_OBSTACLE:
                                dest->df[o] = source->df[i];
                                break;
                            case GF_FLUID:
                                dest->mass += neigh->df[o];
                                dest->mass -= source->df[i];
                                dest->df[o] = neigh->df[o];
                                break;
                            case GF_INTERFACE:
                                dest->df[o] = neigh->df[o];
                                neighcount(gf, x+dx, y+dy, &iemptycount, &ifluidcount);
                                float fluidratio = (source->fluid + neigh->fluid)/2;
                                int tmp1 = emptycount == iemptycount && fluidcount == ifluidcount;
                                float deltamass = 0;
                                if (tmp1 || !emptycount || !ifluidcount)
                                    deltamass += neigh->df[o];
                                if (tmp1 || !iemptycount || !fluidcount)
                                    deltamass -= source->df[i];
                                dest->mass += deltamass * fluidratio;
                                break;
                            case GF_EMPTY:
                                dest->df[o] = eqdf[i] + eqdf[o] - source->df[i];
                                break;
                            default:
                                break;
                        }
                        if (neigh->flags != GF_EMPTY && dot2f(nx, ny, dx, dy) > 0)
                            dest->df[o] = eqdf[i] + eqdf[o] - source->df[i];
                        pressure += dest->df[o];
                        if (dest->df[o] < 0)
                            abort();
                    }
                    dest->fluid = dest->mass/pressure;
                    break;
                case GF_OBSTACLE:
                case GF_EMPTY:
                    *dest = *source;
                    break;
            }
            gf->props->total_mass += dest->mass;
        }
    }
}

static void gridfluid_collide(gridfluid_t gf) {
    float pressure=0;
    float ux=0;
    float uy=0;
    float eq[9];
    gf->filled = 0;
    gf->emptied = 0;
    memset(gf->changeflags, 0, gf->x * gf->y * sizeof(gridfluid_change_flag));
    for (size_t i=0; i < gf->x * gf->y; i++) {
        gridfluid_cell_t *cell = &gf->grid[i];
        if (cell->flags != GF_FLUID && cell->flags != GF_INTERFACE) {
            continue;
        }
        gridfluid_cell_macro(cell->df, &pressure, &ux, &uy);
        if (pressure > 100000)
            abort();
        //uy += gf->gravity;
        gridfluid_eq(pressure, ux, uy, eq);
        gf->props->pressure[i] = pressure;
        gf->props->mass[i] = cell->mass;
        cell->fluid = cell->mass/pressure;
        if (cell->flags == GF_INTERFACE && cell->mass > 1 + change_fudge) {
            gf->changeflags[i] = GF_CHANGE_FILLED;
            gf->filled++;
        }
        if (cell->flags == GF_INTERFACE && cell->mass < 0 - change_fudge) {
            gf->changeflags[i] = GF_CHANGE_EMPTIED;
            gf->emptied++;
        }
        for (size_t j=0; j<9; j++) {
            float f = cell->df[j];
            //float nf = f - omega * (f - eq[j]);
            float nf = f * (1-omega) + omega * eq[j];
            if (nf < 0)
                abort();
            cell->df[j] = nf;
        }
    }
}

static void gridfluid_avg_macro(gridfluid_t gf, size_t x, size_t y, float *pressure, float *ux, float *uy) {
    float tpressure = 0;
    float tux = 0;
    float tuy = 0;
    size_t count = 0;
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            float ipressure;
            float iux;
            float iuy;
            if (dx == 0 && dx == dy)
                continue;
            gridfluid_cell_t *cell = &GF_CELL(gf, x+dx, y+dy);
            switch(cell->flags) {
                case GF_FLUID:
                case GF_INTERFACE:
                    gridfluid_cell_macro(cell->df, &ipressure, &iux, &iuy);
                    count++;
                    tpressure += ipressure;
                    tux += iux;
                    tuy += iuy;
                    break;
                default:
                    break;
            }
        }
    }
    if (count == 0)
        return;
    *pressure = tpressure / count;
    *ux = tux / count;
    *uy = tuy / count;
}

static void distribmass(gridfluid_t gf, size_t x, size_t y) {
    float partials[9];
    float total = 0;
    float nx=0;
    float ny=0;
    gridfluid_cell_normal(gf, x, y, &nx, &ny);
    gridfluid_change_flag change = gf->changeflags[x + y*gf->x];
    gridfluid_cell_t *source = &GF_CELL(gf, x, y);
    int mul = (change == GF_CHANGE_FILLED) ? 1 : -1;
    float mass = 0;
    float pressure, ux, uy;
    if (change == GF_CHANGE_FILLED) {
        gridfluid_cell_macro(source->df, &pressure, &ux, &uy);
        mass = source->mass - pressure;
    } else if (change == GF_CHANGE_EMPTIED) {
        mass = source->mass;
    }
    for (size_t i=0; i<9; i++) {
        int dx = velocities[i][0];
        int dy = velocities[i][1];
        if (dx == 0 && dx == dy)
            continue;
        gridfluid_cell_t *neigh = &GF_CELL(gf, x+dx, y+dy);
        if (neigh->flags != GF_INTERFACE)
            continue;
        float n_e = dot2f(nx,ny,dx,dy) * mul;
        if (n_e > 0) {
            total += n_e;
            partials[i] = n_e;
        }

    }
    for (size_t i=0; i<9; i++) {
        int dx = velocities[i][0];
        int dy = velocities[i][1];
        gridfluid_cell_t *neigh = &GF_CELL(gf, x+dx, y+dy);
        gridfluid_cell_macro(neigh->df, &pressure, &ux, &uy);
        if (dx == 0 && dx == dy) {
            source->mass -= mass;
            gf->props->total_mass -= mass;
            source->fluid = source->mass/pressure;
            continue;
        }
        if (neigh->flags != GF_INTERFACE)
            continue;
        float deltamass = mass * partials[i]/total;
        neigh->mass += deltamass;
        gf->props->total_mass += deltamass;
        neigh->fluid = neigh->mass/pressure;
    }
}

static void gridfluid_cleanup(gridfluid_t gf) {
    for (size_t x = 0; x < gf->x; x++) {
        for (size_t y = 0; y < gf->y; y++) {
            gridfluid_cell_t *cell = &GF_CELL(gf,x,y);
            if (gf->changeflags[x + y*gf->x] != GF_CHANGE_FILLED)
                continue;
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    gridfluid_cell_t *neigh = &GF_CELL(gf,x+dx,y+dy);
                    if (neigh->flags == GF_INTERFACE)
                        gf->changeflags[x+y*gf->x] = GF_CHANGE_NONE;
                    if (neigh->flags != GF_EMPTY)
                        continue;
                    float pressure = 0;
                    float ux = 0;
                    float uy = 0;
                    float eq[9];
                    gridfluid_avg_macro(gf,x+dx,y+dy,&pressure,&ux,&uy);
                    gridfluid_eq(pressure, ux, uy, eq);
                    neigh->flags = GF_INTERFACE;
                    memcpy(neigh->df, eq, 9*sizeof(float));
                }
            }
            cell->flags = GF_FLUID;
        }
    }
    for (size_t x = 0; x < gf->x; x++) {
        for (size_t y = 0; y < gf->y; y++) {
            gridfluid_cell_t *cell = &GF_CELL(gf,x,y);
            if (gf->changeflags[x + y*gf->x] != GF_CHANGE_EMPTIED)
                continue;
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    gridfluid_cell_t *neigh = &GF_CELL(gf,x+dx,y+dy);
                    if (neigh->flags != GF_FLUID)
                        continue;
                    neigh->flags = GF_INTERFACE;
                }
            }
            cell->flags = GF_EMPTY;
        }
    }
    for (size_t x = 0; x < gf->x; x++) {
        for (size_t y = 0; y < gf->y; y++) {
            //gridfluid_cell_t *cell = &GF_CELL(gf,x,y);
            if (gf->changeflags[x + y*gf->x] == GF_CHANGE_NONE)
                continue;
            distribmass(gf,x,y);
            gf->changeflags[x + y*gf->x] = GF_CHANGE_NONE;
        }
    }
}


gridfluid_t gridfluid_create_empty_scene(size_t x, size_t y) {
    gridfluid_t gf = calloc(1, sizeof(struct gridfluid));
    gf->props = calloc(1, sizeof(struct gridfluid_properties));
    gf->x = x;
    gf->y = y;
    gf->gravity = 0.01;
    gf->atmosphere = 1.0;
    gf->grid = calloc(x*y, sizeof(gridfluid_cell_t));
    gf->nextgrid = calloc(x*y, sizeof(gridfluid_cell_t));
    gf->changeflags = calloc(x*y, sizeof(gridfluid_change_flag));
    gf->props->x = x;
    gf->props->y = y;
    gf->props->pressure = calloc(x*y, sizeof(float));
    gf->props->mass = calloc(x*y, sizeof(float));
    for (size_t i = 1; i < x-1; i++) {
        for (size_t j = 1; j < y-1; j++) {
            GF_CELL(gf,i,j).flags = GF_EMPTY;
            /*
            gridfluid_set_fluid(gf, i, j);
            if (i == x/3 || i < 4 || i >= x-4 || j < 4 || j >= y-4)
                GF_CELL(gf,i,j).flags = GF_INTERFACE;
            if (i > x/3 || i < 3 || i >= x-3 || j < 3 || j >= y-3)
                GF_CELL(gf,i,j).flags = GF_EMPTY;
            //GF_CELL(gf,i,j).flags = GF_EMPTY;
            */
        }
    }
    return(gf);
}

void gridfluid_free(gridfluid_t gf) {
    free(gf->grid);
    free(gf->nextgrid);
    free(gf->changeflags);
    free(gf->props->pressure);
    free(gf->props->mass);
    free(gf->props);
    free(gf);
}

void gridfluid_set_obstacle(gridfluid_t gf, size_t x, size_t y) {
    GF_CELL(gf,x,y).flags = GF_OBSTACLE;
}

void gridfluid_set_fluid(gridfluid_t gf, size_t x, size_t y) {
    gridfluid_cell_t *cell = &GF_CELL(gf,x,y);
    gridfluid_eq(1,0.00,0.00,cell->df);
    cell->flags = GF_FLUID;
    cell->mass = 1;
    cell->fluid = 1;
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            if (dx == 0 && dx == dy)
                continue;
            gridfluid_cell_t *neigh = &GF_CELL(gf,x+dx,y+dy);
            if (neigh->flags == GF_EMPTY) {
                gridfluid_eq(1,0.00,0.00,neigh->df);
                neigh->flags = GF_INTERFACE;
                neigh->mass = 0.9;
                neigh->fluid = 0.9;
            }
        }
    }
}

void gridfluid_set_empty(gridfluid_t gf, size_t x, size_t y) {
    GF_CELL(gf,x,y).flags = GF_EMPTY;
}

void gridfluid_set_gravity(gridfluid_t gf, float g) {
    gf->props->gravity = g;
}

uint8_t gridfluid_get_type(gridfluid_t gf, size_t x, size_t y) {
    return( GF_CELL(gf,x,y).flags );
}

gridfluid_properties_t *gridfluid_get_properties(gridfluid_t gf) {
    float pressure=0;
    float ux=0;
    float uy=0;
    float max_pressure = 0;
    float min_pressure = INFINITY;
    float max_usqr = 0;
    for (size_t i=0; i < gf->x * gf->y; i++) {
        gridfluid_cell_t *cell = &gf->grid[i];
        if (cell->flags != GF_FLUID && cell->flags != GF_INTERFACE) {
            continue;
        }
        gridfluid_cell_macro(cell->df, &pressure, &ux, &uy);
        float usqr = ux*ux + uy*uy;
        gf->props->pressure[i] = pressure;
        gf->props->mass[i] = cell->mass;
        cell->fluid = cell->mass/pressure;
        if (pressure > max_pressure)
            max_pressure = pressure;
        if (pressure < min_pressure)
            min_pressure = pressure;
        if (usqr > max_usqr)
            max_usqr = usqr;
    }
    gf->props->min_pressure = min_pressure;
    gf->props->max_pressure = max_pressure;
    gf->props->max_velocity = sqrtf(max_usqr);
    return( gf->props );
}

void gridfluid_debug(gridfluid_t gf) {
    gf->props->total_mass = 0;
    for (size_t x = 0; x < gf->x; x++) {
        for (size_t y = 0; y < gf->y; y++) {
            gf->props->total_mass += GF_CELL(gf,x,y).mass;
        }
    }
    printf("total mass: %f           \n", gf->props->total_mass);
}

void gridfluid_step(gridfluid_t gf) {
    gf->props->total_mass = 0;
    gridfluid_stream(gf);
    gridfluid_cell_t *tmp = gf->grid;
    gf->grid = gf->nextgrid;
    gf->nextgrid = tmp;
    gridfluid_collide(gf);
    gridfluid_cleanup(gf);
}
