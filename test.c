#include <stdio.h>
#include <npraises.h>

int main() {
    setup_screen();
    set_fg(rgb_f(1,0,0));
    set_bg(rgb_f(0,0,1));
    printf("ohai\n");
    reset_colors();
    return 0;
}
