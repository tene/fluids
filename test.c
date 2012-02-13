#include <stdio.h>
#include <npraises.h>
#include <unistd.h>

int main() {
    if (!setup_screen()) {
        printf("Failed to setup screen; exiting\n");
        return(1);
    }
    sleep(1);
    set_fg(rgb_f(1,1,1));
    set_bg(rgb_f(0,0,0));
    curs_xy(30,5);
    printf("lol\n");
    set_fg(rgb_f(1,0,0));
    set_bg(rgb_f(0,0,1));
    curs_xy(100,5);
    printf("ohai\n");
    sleep(3);
    cleanup_screen();
    return 0;
}
