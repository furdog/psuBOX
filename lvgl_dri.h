#include <lvgl.h>
#include <TFT_eSPI.h>

/* Calibration data for tft-espi display (TODO calibrate
 * 					  on first run automatically). */
uint16_t calData[5] = { 320, 3420, 380, 3480, 1 };

lv_display_t *lvgl_dri_disp;

/*Set to your screen resolution*/
#define TFT_HOR_RES   240
#define TFT_VER_RES   320
#define TFT_ROTATION  LV_DISPLAY_ROTATION_90

/* LVGL draw into this buffer, 1/10 screen size usually works well.
 * The size is in bytes*/
#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 10 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

#if LV_USE_LOG != 0
void my_print( lv_log_level_t level, const char * buf )
{
	LV_UNUSED(level);
	Serial.println(buf);
	Serial.flush();
}
#endif

/* LVGL calls it when a rendered image needs to copied to the display */
void my_disp_flush(lv_display_t *disp, const lv_area_t *area,
		   uint8_t * px_map)
{
	lv_disp_flush_ready(disp);
}

/*Read the touchpad*/
void my_touchpad_read(lv_indev_t * indev, lv_indev_data_t * data)
{
	/*For example  ("my_..." functions needs to be implemented by you) */
	uint16_t x, y;

	TFT_eSPI *tft = *((TFT_eSPI **)
				lv_display_get_driver_data(lvgl_dri_disp));

	// read the touchpad
	bool touched = tft->getTouch(&x, &y, 600);

	if(!touched) {
		data->state = LV_INDEV_STATE_RELEASED;
	} else {
		/*Serial.print( "touch x: " );
		  Serial.println( x );
		  Serial.print( "touch y: " );
		  Serial.println( y );*/

		data->state = LV_INDEV_STATE_PRESSED;

		/* Bug in lvgl code makes touch rotated by 90deg... */
		/* It's actually point.y */

		//Uncomment in case if touch is not rotated by 180
		//data->point.x = TFT_HOR_RES - 1 - y;
		//data->point.y = x; /* It's actually point.x */

		/** In case if touch is rotated by 180 deg */
		data->point.x = y;
		data->point.y = TFT_VER_RES - 1 - x;
	}
}

void lvgl_dri_setup()
{
	String LVGL_Arduino = "Hello Arduino! ";
	LVGL_Arduino += String('V') + lv_version_major() + "." +
			lv_version_minor() + "." + lv_version_patch();

	Serial.println( LVGL_Arduino );

	lv_init();

	/*Set a tick source so that LVGL will know how much time elapsed. */
	lv_tick_set_cb(millis);

	/* register print function for debugging */
	#if LV_USE_LOG != 0
		lv_log_register_print_cb( my_print );
	#endif

	lvgl_dri_disp = lv_tft_espi_create(TFT_HOR_RES, TFT_VER_RES,
					   draw_buf, sizeof(draw_buf));
	lv_display_set_rotation(lvgl_dri_disp, TFT_ROTATION);

	/* Initialize the (dummy) input device driver */
	lv_indev_t * indev = lv_indev_create();
	lv_indev_set_long_press_time(indev, PSU_BOX_ARM_DURATION);

	/* Touchpad should have POINTER type */
	lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
	lv_indev_set_read_cb(indev, my_touchpad_read);

	TFT_eSPI *tft = *((TFT_eSPI **)
				lv_display_get_driver_data(lvgl_dri_disp));

	tft->setTouch(calData);
}

void lvgl_dri_update()
{
	lv_timer_handler(); /* let the GUI do its work */
}
