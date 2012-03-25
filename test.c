#include "npraises.h"
#include "gridfluid.h"
#include <stdio.h>
#include <unistd.h>
#include <assert.h>

void render(gridfluid_t gf) {
    float mp = gridfluid_get_max_pressure(gf);
    float pressure;
    set_bg(rgb_f(0,0,0));
    for (int x=0; x<40; x++) {
        for (int y=0; y<20; y++) {
            curs_xy(x,y);
            switch(gridfluid_get_type(gf,x,y)) {
                case GF_OBSTACLE:
                    set_fg(rgb_f(1,1,1));
                    set_bg(rgb_f(1,1,1));
                    break;
                case GF_EMPTY:
                    set_fg(rgb_f(0,0,0));
                    set_bg(rgb_f(0,0,0));
                    break;
                case GF_FLUID:
                    pressure = gridfluid_get_pressure(gf,x,y);
                    set_fg(rgb_f(0,0,pressure/mp));
                    set_bg(rgb_f(0,0,pressure/mp));
                    break;
                case GF_INTERFACE:
                    set_fg(rgb_f(0,0,0.5));
                    set_bg(rgb_f(0,0,0.5));
                    break;
            }
            printf("█");
        }
    }
}

int main() {
    gridfluid_t gf = gridfluid_create_empty_scene(40,20);
    gridfluid_set_gravity(gf,1.0f);
    gridfluid_set_obstacle(gf,5,7);
    for (size_t i=7; i<40; i++) {
        gridfluid_set_obstacle(gf,i,12);
    }
    /*
    gridfluid_set_fluid(gf,10,10);
    assert(gridfluid_get_type(gf,5,7) == GF_OBSTACLE);
    */
    if (!setup_screen()) {
        printf("Failed to setup screen; exiting\n");
        return(1);
    }

    ssize_t len;
    unsigned char buf[1];
    int running = 1;
    while (running) {
        gridfluid_step(gf);
        render(gf);
        float mp = gridfluid_get_max_pressure(gf);
        set_fg(rgb_f(1,1,1));
        set_bg(rgb_f(0,0,0));

        curs_xy(10,28);
        printf("max pressure: %f     ", mp);
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
