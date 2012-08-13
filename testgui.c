#define _GNU_SOURCE
#include <Elementary.h>

static void
on_done(void *data, Evas_Object *obj, void *event_info) {
    elm_exit();
}

EAPI_MAIN int
elm_main(int argc, char **argv) {
    Evas_Object *win, *mainbox, *sidebox, *label, *button, *grid, *gridframe;

    win = elm_win_util_standard_add("fluidtest", "Fluids Test");
    evas_object_smart_callback_add(win, "delete,request", on_done, NULL);

    mainbox = elm_box_add(win);
    elm_box_horizontal_set(mainbox, EINA_TRUE);
    elm_box_homogeneous_set(mainbox, EINA_FALSE);
    elm_win_resize_object_add(win, mainbox);
    evas_object_size_hint_weight_set(mainbox, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_show(mainbox);

    grid = evas_object_rectangle_add(evas_object_evas_get(win));
    elm_win_resize_object_add(win, grid);
    evas_object_size_hint_weight_set(grid, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_min_set(grid, 30, 30);
    evas_object_size_hint_align_set(grid, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_color_set(grid, 0, 0, 255, 255);
    //elm_box_pack_start(mainbox, grid);
    evas_object_show(grid);

    gridframe = elm_frame_add(win);
    elm_object_content_set(gridframe, grid);
    elm_object_text_set(gridframe, "Step 0");
    elm_box_pack_start(mainbox, gridframe);
    //elm_win_resize_object_add(win, gridframe);
    evas_object_size_hint_weight_set(gridframe, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    //evas_object_size_hint_min_set(grid, 30, 30);
    evas_object_size_hint_align_set(gridframe, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_show(gridframe);

    sidebox = elm_box_add(win);
    elm_box_horizontal_set(sidebox, EINA_FALSE);
    //elm_win_resize_object_add(win, sidebox);
    //evas_object_size_hint_weight_set(sidebox, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_weight_set(sidebox, 0, 0);

    label = elm_label_add(win);
    elm_object_text_set(label, "Fluids Test");
    elm_box_pack_end(sidebox, label);
    evas_object_show(label);

    button = elm_button_add(win);
    elm_object_text_set(button, "Exit");
    elm_box_pack_end(sidebox, button);
    evas_object_show(button);

    elm_box_pack_end(mainbox, sidebox);
    evas_object_show(sidebox);

    evas_object_smart_callback_add(button, "clicked", on_done, NULL);

    evas_object_show(win);

    elm_run(); // run main loop
    elm_shutdown(); // after mainloop finishes running, shutdown
    return 0; // exit 0 for exit code
}
ELM_MAIN()
