#include "npraises.h"
#include "gridfluid.h"
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <stdbool.h>

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
                    draw = "I";
                    break;
            }
            printf("%s", draw);
        }
    }
}

int main() {
    size_t scene_x = 40;
    size_t scene_y = 20;
    gridfluid_t gf = gridfluid_create_empty_scene(scene_x, scene_y);
    //gridfluid_set_obstacle(gf,5,7);
    for (size_t i=9; i<40; i++) {
        gridfluid_set_obstacle(gf,i,12);
    }
    gridfluid_set_gravity(gf, 0.01);
    for (size_t fy = 3; fy < 10; fy++) {
        for (size_t fx = 4; fx < 7; fx++) {
            gridfluid_set_fluid(gf,fx, fy);
        }
    }
    for (size_t fy = 3; fy < 5; fy++) {
        for (size_t fx = 4; fx < 20; fx++) {
            gridfluid_set_fluid(gf,fx, fy);
        }
    }
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
    unsigned int step_count = 0;
    unsigned int steps_per_frame = 1;
    gridfluid_properties_t *props;
    size_t debug_x = scene_x/6;
    size_t debug_y = scene_y/2;

    bool simulating = false;
    bool debugging = false;
    while (running) {
        if (simulating) {
            for (size_t i = 0; i < simulating * steps_per_frame; i++) {
                curs_xy(10,31);
                gridfluid_step(gf);
                step_count++;
            }
            render(gf);
        }
        props = gridfluid_get_properties(gf);
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
        printf("step count: %d", step_count);
        curs_xy(10,30);
        printf("debugging: %s", debugging ? "on ": "off");
        if (debugging) {
            float mass = props->mass[debug_x + debug_y * scene_x];
            float pressure = props->pressure[debug_x + debug_y * scene_x];
            curs_xy(50,10);
            printf("x: %zu, y: %zu", debug_x, debug_y);
            curs_xy(50,11);
            printf("mass: %f, pressure: %f", mass, pressure);
            curs_xy(debug_x, debug_y);
            set_fg(rgb_f(1,0,0));
            printf("█");
            set_fg(rgb_f(1,1,1));
        }
        curs_xy(10,31);

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
                    step_count++;
                    render(gf);
                    break;
                case 'd':
                    debugging = debugging ? false : true;
                    break;
                case 'h':
                    debug_x--;
                    break;
                case 'j':
                    debug_y++;
                    break;
                case 'k':
                    debug_y--;
                    break;
                case 'l':
                    debug_x++;
                    break;
                case ' ':
                    simulating = simulating ? false : true;
                    break;
                default:
                    printf("read: %3u        ", buf[0]);
                    break;
            }
            //clear();
            //render(gf);
        }
    }
    cleanup_screen();
    gridfluid_free(gf);
    return 0;
}
