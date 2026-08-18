#include "twai_dri.h"
#include "psu_box_ui.h"
struct psu_box_ui ui;

/******************************************************************************
 * PSU
 *****************************************************************************/
#define PSU_FAN_CONTROL_PIN 25

char psu_dev_info_str[0xFF];
bool psu_dev_info_conn_established = false;

//#define TBCM_360_3000_HE_DRI_LOG(v) {printf v;}
#include "tbcm_360_3000_he_dri.h"
#include "delta_time.h"
#include "fan_ctl.h"

#define CHARGE_COUNTER_IMPL
#include "charge_counter.h"

struct delta_time dt;
struct tbcm_360_3000_he_dri tbcm_dri;

/* Charge counter (coloumb counter) watt/hours */
struct chgc chgc;
struct chgc_config chgc_cfg;

void setup_psuBOX()
{
	pinMode(PSU_FAN_CONTROL_PIN, OUTPUT);
	digitalWrite(PSU_FAN_CONTROL_PIN, LOW);

	Serial.begin(921600);

	esp32_twai_init(&twai_bus_0);
	//esp32_twai_init(&twai_bus_1);

	delta_time_init(&dt);
	tbcm_360_3000_he_dri_init(&tbcm_dri);

	psu_dev_info_str[0] = '\0';

	/* Charger counter config */
	chgc_init(&chgc);

	chgc_cfg.max_energy_wh      = 99999;
	chgc_cfg.update_interval_ms = 100u;
	chgc_cfg.multiplier_V       = 10u;
	chgc_cfg.multiplier_A       = 10u;
	chgc_cfg.report_timeout_ms  = 500u;

	chgc_set_config(&chgc, chgc_cfg);
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

	/* Control fan speed based on temp. See fan_ctl.h for detail */
	uint16_t pwm_duty = fan_control(
			tbcm_360_3000_he_dri_get_out_temp1(&tbcm_dri),
			tbcm_360_3000_he_dri_get_out_temp2(&tbcm_dri));

	if (psu_dev_info_conn_established &&	
	    /* Dirty hack to prevent 0xFFFF values...
	     * Temporary solution, must be fixed properly in
	     * tbcm driver. TODO, FIXME */
	    ((tbcm_dri._reader.rflags & 8U) > 0U)) {
		chgc_set_voltage_V(&chgc, 10.0f *
			tbcm_360_3000_he_dri_get_out_voltage_V(&tbcm_dri));
		chgc_set_current_A(&chgc, 10.0f *
			tbcm_360_3000_he_dri_get_out_current_A(&tbcm_dri));
		chgc_update(&chgc, delta_time_ms); 
	}

	analogWrite(PSU_FAN_CONTROL_PIN, pwm_duty);
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

		ui.total_wh = chgc_get_energy_wh(&chgc);
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
