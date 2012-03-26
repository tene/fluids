#include "npraises.h"
#include "gridfluid.h"
#include <stdio.h>
#include <unistd.h>
#include <assert.h>

void render(gridfluid_t gf) {
    gridfluid_properties_t *props = gridfluid_get_properties(gf);
    float maxp = props->max_pressure;
    float minp = props->min_pressure;
    float pressure;
    char *draw = " ";
    set_bg(rgb_f(0,0,0));
    for (int x=0; x < props->x; x++) {
        for (int y=0; y < props->y; y++) {
            curs_xy(x,y);
            switch(gridfluid_get_type(gf,x,y)) {
                case GF_OBSTACLE:
                    set_fg(rgb_f(1,1,1));
                    set_bg(rgb_f(1,1,1));
                    draw = "█";
                    break;
                case GF_EMPTY:
                    set_fg(rgb_f(0,0,0));
                    set_bg(rgb_f(0,0,0));
                    draw = "█";
                    break;
                case GF_FLUID:
                    pressure = props->pressure[x + y*props->x];
                    set_fg(rgb_f(0,0,0.2 + 0.8*(pressure-minp)/(maxp-minp)));
                    set_bg(rgb_f(0,0,0.2 + 0.8*(pressure-minp)/(maxp-minp)));
                    break;
                case GF_INTERFACE:
                    pressure = props->pressure[x + y*props->x];
                    set_fg(rgb_f(0,0,0.2 + 0.8*(pressure-minp)/(maxp-minp)));
                    set_bg(rgb_f(0,0,0));
                    draw = "▓";
                    break;
            }
            printf("%s", draw);
        }
    }
}

int main() {
    gridfluid_t gf = gridfluid_create_empty_scene(40,20);
    gridfluid_set_obstacle(gf,5,7);
    for (size_t i=7; i<40; i++) {
        gridfluid_set_obstacle(gf,i,12);
    }
    gridfluid_set_gravity(gf, 0.01);
    gridfluid_set_fluid(gf,10,10);
    /*
    assert(gridfluid_get_type(gf,5,7) == GF_OBSTACLE);
    */
    if (!setup_screen()) {
        printf("Failed to setup screen; exiting\n");
        return(1);
    }

    ssize_t len;
    unsigned char buf[1];
    int running = 1;
    unsigned int frame_count = 0;
    unsigned int steps_per_frame = 1;
    int unpaused = 0;
    gridfluid_properties_t *props;
    while (running) {
        for (size_t i = 0; i < unpaused * steps_per_frame; i++) {
            gridfluid_step(gf);
        }
        props = gridfluid_get_properties(gf);
        render(gf);
        float maxp = props->max_pressure;
        float minp = props->min_pressure;
        float maxv = props->max_velocity;
        set_fg(rgb_f(1,1,1));
        set_bg(rgb_f(0,0,0));

        curs_xy(10,25);
        printf("min pressure: %f     ", minp);
        curs_xy(10,26);
        printf("max pressure: %f     ", maxp);
        curs_xy(10,27);
        printf("max velocity: %f     ", maxv);
        curs_xy(10,28);
        printf("total mass: %f     ", props->total_mass);
        curs_xy(10,29);
        printf("step count: %d", frame_count++ * steps_per_frame);
        curs_xy(10,30);

        len = read(0, buf, 1);
        if (len == -1) {
            perror("read: ");
        } else if (len == 1) {
            switch(buf[0]) {
                case 3:
                case 'q':
                    running = 0;
                    break;
                case 's':
                    gridfluid_step(gf);
                    break;
                case ' ':
                    unpaused = unpaused ? 0 : 1;
                    break;
                default:
                    printf("read: %3u        ", buf[0]);
                    break;
            }
        }
    }
    cleanup_screen();
    gridfluid_free(gf);
    return 0;
}
