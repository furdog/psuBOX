#include "twai_dri.h"
#include "psu_box_ui.h"
struct psu_box_ui ui;

/******************************************************************************
 * PSU
 *****************************************************************************/
char psu_dev_info_str[0xFF];
bool psu_dev_info_conn_established = false;

//#define TBCM_360_3000_HE_DRI_LOG(v) {printf v;}
#include "tbcm_360_3000_he_dri.h"
#include "delta_time.h"

struct delta_time dt;
struct tbcm_360_3000_he_dri tbcm_dri;

void setup_psuBOX()
{
	Serial.begin(921600);

	esp32_twai_init(&twai_bus_0);
	//esp32_twai_init(&twai_bus_1);

	delta_time_init(&dt);
	tbcm_360_3000_he_dri_init(&tbcm_dri);

	psu_dev_info_str[0] = '\0';
}

void loop_psuBOX()
{
	enum tbcm_360_3000_he_dri_event tbcm_ev;
	uint32_t delta_time_ms = delta_time_update_ms(&dt, millis());
	struct tbcm_360_3000_he_dri_frame frame;

	esp32_twai_update();

	if (esp32_twai_recv_frame(0, (struct esp32_twai_frame *)&frame)) {
		tbcm_360_3000_he_dri_write_frame(&tbcm_dri, &frame);
	}

	if (tbcm_360_3000_he_dri_read_frame(&tbcm_dri, &frame)) {
		esp32_twai_send_frame(0, (struct esp32_twai_frame *)&frame);
	}

	tbcm_ev = tbcm_360_3000_he_dri_update(&tbcm_dri, delta_time_ms);

	switch (tbcm_ev) {
	case TBCM_360_3000_HE_DRI_EVENT_NONE:
		break;

	case TBCM_360_3000_HE_DRI_EVENT_SERIAL_NO:
		snprintf(psu_dev_info_str, 0xFF,
			"ev: SERIAL_NO\n"
			"no: %s", tbcm_360_3000_he_dri_get_serial_no(
								   &tbcm_dri));

		tbcm_360_3000_he_dri_accept_serial_no(&tbcm_dri);
		break;

	case TBCM_360_3000_HE_DRI_EVENT_DEVICE_ID:
		snprintf(psu_dev_info_str, 0xFF,
			"ev: DEVICE_ID\n"
			"no: %s\nid: %i", tbcm_360_3000_he_dri_get_serial_no(
								   &tbcm_dri),
			tbcm_360_3000_he_dri_get_device_id(&tbcm_dri));

		tbcm_360_3000_he_dri_accept_device_id(&tbcm_dri);

		/* Set INITIAL settings */
		tbcm_360_3000_he_dri_set_defaults(&tbcm_dri);
		tbcm_360_3000_he_dri_set_voltage_V(&tbcm_dri,
						   ui.set_voltage_V);
		tbcm_360_3000_he_dri_set_current_A(&tbcm_dri,
						   ui.set_current_A);
		tbcm_360_3000_he_dri_set_charging_mode(&tbcm_dri,
						ui.charging_enabled ? 1U : 0U);
		break;

	case TBCM_360_3000_HE_DRI_EVENT_ESTABLISHED:
		psu_dev_info_conn_established = true;
		snprintf(psu_dev_info_str, 0xFF,
			"ev: ESTABLISHED\n"
			"no: %s\nid: %i", tbcm_360_3000_he_dri_get_serial_no(
								   &tbcm_dri),
			tbcm_360_3000_he_dri_get_device_id(&tbcm_dri));
		break;

	case TBCM_360_3000_HE_DRI_EVENT_FAULT:
		psu_dev_info_conn_established = false;
		snprintf(psu_dev_info_str, 0xFF,
			"ev: FAULT\n"
			"no: %s\nid: %i", tbcm_360_3000_he_dri_get_serial_no(
								   &tbcm_dri),
			tbcm_360_3000_he_dri_get_device_id(&tbcm_dri));
		break;

	default:
		break;
	}
}

/******************************************************************************
 * DISPLAY
 *****************************************************************************/
#include "lvgl_dri.h"

void setup()
{
	lvgl_dri_setup();
	psu_box_ui_init(&ui);
	ui.min_voltage_V = 250;
	ui.max_voltage_V = 420;
	ui.min_current_A = 0;
	ui.max_current_A = 10;
	psu_box_ui_update(&ui);

	setup_psuBOX();
	strcpy(psu_dev_info_str, "Device info is not available");
}

void loop(void)
{
	/* Update ui */
	if (psu_dev_info_conn_established) {
		ui.out_voltage_V =
			tbcm_360_3000_he_dri_get_out_voltage_V(&tbcm_dri);
		ui.out_current_A =
			tbcm_360_3000_he_dri_get_out_current_A(&tbcm_dri);
	} else {
		ui.out_voltage_V = 0;
		ui.out_current_A = 0;
	}
	psu_box_ui_stats_update(&ui);
	psu_box_ui_info_update(&ui, psu_dev_info_str);

	lvgl_dri_update();

	/* Read settings from ui */
	tbcm_360_3000_he_dri_set_voltage_V(&tbcm_dri, ui.set_voltage_V);
	tbcm_360_3000_he_dri_set_current_A(&tbcm_dri, ui.set_current_A);
	tbcm_360_3000_he_dri_set_charging_mode(&tbcm_dri,
						ui.charging_enabled ? 1U : 0U);

	loop_psuBOX();
}
