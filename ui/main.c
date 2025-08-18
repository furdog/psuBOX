/**
 * @file main
 *
 */

#define _DEFAULT_SOURCE /* needed for usleep() */
#include <stdlib.h>
#include <unistd.h>
#include "lvgl.h"

#include "psu_box_ui.h"
struct psu_box_ui ui;

static lv_display_t * hal_init(int32_t w, int32_t h);

int main(int argc, char **argv)
{
	(void)argc; /*Unused*/
	(void)argv; /*Unused*/

	/*Initialize LVGL*/
	lv_init();

	/*Initialize the display, and the input devices*/
	hal_init(320, 240);

	/*Open a demo or an example*/
	/* if (argc == 0) {
		lv_demo_widgets();
		//  lv_example_chart_1();
	} else {
		if (!lv_demos_create(&argv[1], argc - 1)) {
			lv_demos_show_help();
			goto demo_end;
		}
	} */
	
	psu_box_ui_init(&ui);
	ui.min_voltage_V = 250;
	ui.max_voltage_V = 420;
	ui.min_current_A = 0;
	ui.max_current_A = 10;
	psu_box_ui_update(&ui);

	/*To hide the memory and performance indicators in the corners
	 *disable `LV_USE_MEM_MONITOR` and `LV_USE_PERF_MONITOR` in `lv_conf.h`*/

	while(1) {
		/* Periodically call the lv_task handler.
		 * It could be done in a timer interrupt or an OS task too.*/
		lv_timer_handler();
		usleep(10* 1000);
		
		/* tbms_ui_set_default(); */
	}

//demo_end:
	lv_deinit();
	return 0;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/**
 * Initialize the Hardware Abstraction Layer (HAL) for the LVGL graphics
 * library
 */
static lv_display_t * hal_init(int32_t w, int32_t h)
{
	lv_group_set_default(lv_group_create());

	lv_display_t * disp = lv_sdl_window_create(w, h);

	lv_indev_t * mouse = lv_sdl_mouse_create();
	lv_indev_set_group(mouse, lv_group_get_default());
	lv_indev_set_display(mouse, disp);
	lv_display_set_default(disp);

	lv_indev_t * mousewheel = lv_sdl_mousewheel_create();
	lv_indev_set_display(mousewheel, disp);

	lv_indev_t * keyboard = lv_sdl_keyboard_create();
	lv_indev_set_display(keyboard, disp);
	lv_indev_set_group(keyboard, lv_group_get_default());

	return disp;
}
