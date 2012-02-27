#include "npraises.h"
#include "gridfluid.h"
#include <stdio.h>
#include <unistd.h>
#include <assert.h>

int main() {
    gridfluid_t gf = gridfluid_create_empty_scene(40,20);
    gridfluid_set_obstacle(gf,5,7);
    gridfluid_set_fluid(gf,10,10);
    gridfluid_set_gravity(gf,1.0f);
    assert(gridfluid_get_type(gf,5,7) == GF_OBSTACLE);
    if (!setup_screen()) {
        printf("Failed to setup screen; exiting\n");
        return(1);
    }
    set_fg(rgb_f(1,1,1));
    set_bg(rgb_f(0,0,0));
    curs_xy(30,5);
    printf("lol\n");
    set_fg(rgb_f(1,0,0));
    set_bg(rgb_f(0,0,1));
    curs_xy(100,5);
    printf("ohai\n");

    set_fg(rgb_f(1,1,1));
    set_bg(rgb_f(0,0,0));

    ssize_t len;
    unsigned char buf[1];
    int running = 1;
    curs_xy(10,10);
    while (running) {
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
                    //printf("read: %3u\n", buf[0]);
                    break;
            }
        }
    }
    cleanup_screen();
    gridfluid_free(gf);
    return 0;
}
