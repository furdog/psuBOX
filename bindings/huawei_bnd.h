#include "psu_box_ui.h"
#include "twai_dri.h"
#include <cstdint>
#include <stdint.h>
struct psu_box_ui ui;

/******************************************************************************
 * PSU
 *****************************************************************************/
char psu_dev_info_str[0xFF];
bool psu_dev_info_conn_established = false;

#include "delta_time.h"
#include "huawei_psu.h"

#define CHARGE_COUNTER_IMPL
#include "charge_counter.h"

struct delta_time dt;
struct huapsu	  huapsu_dri;

/* Charge counter (coloumb counter) watt/hours */
struct chgc	   chgc;
struct chgc_config chgc_cfg;

void setup_psuBOX()
{
	Serial.begin(921600);

	esp32_twai_init(&twai_bus_0, 125u);
	// esp32_twai_init(&twai_bus_1);

	delta_time_init(&dt);
	huapsu_init(&huapsu_dri, HUAPSU_HW_VARIANT_R33640G1);

	psu_dev_info_str[0] = '\0';

	/* Charger counter config */
	chgc_init(&chgc);

	chgc_cfg.max_energy_wh	    = 99999;
	chgc_cfg.update_interval_ms = 100u;
	chgc_cfg.multiplier_V	    = 10u;
	chgc_cfg.multiplier_A	    = 10u;
	chgc_cfg.report_timeout_ms  = 500u;

	chgc_set_config(&chgc, chgc_cfg);
}

void loop_psuBOX()
{
	uint8_t iter;

	huapsu_val_entry voltage;
	huapsu_val_entry current;

	uint32_t delta_time_ms = delta_time_update_ms(&dt, millis());
	struct huapsu_frame frame;

	esp32_twai_update();

	/* Bounded loop to prevent deadlock */
	for (iter = 0U; iter < 10U; iter++) {
		if (huapsu_pop_tx_frame(&huapsu_dri, &frame)) {
			esp32_twai_send_frame(
			    0, (struct esp32_twai_frame *)&frame, true);
		}
	}

	/** We can only receive one message at a time */
	if (esp32_twai_recv_frame(0, (struct esp32_twai_frame *)&frame)) {
		huapsu_push_rx_frame(&huapsu_dri, &frame);
	}

	/* Bounded loop to prevent deadlock */
	for (iter = 0U; iter < 10U; iter++) {
		uint8_t event;

		event = huapsu_step(&huapsu_dri);
		if (event == HUAPSU_EVENT_REQ_TIME) {
			huapsu_update_timers(&huapsu_dri, delta_time_ms);
		} else if (event == HUAPSU_EVENT_ONLINE) {
			snprintf(psu_dev_info_str, 0xFF, "STATUS: OK");
		} else if (event == HUAPSU_EVENT_OFFLINE) {
			snprintf(psu_dev_info_str, 0xFF, "OFFLINE");
		}

		if (huapsu_yield(&huapsu_dri)) {
			break;
		}
	}

	if (huapsu_dri.online &&
	    huapsu_get_data_value(&huapsu_dri, HUAPSU_DATA_ID_OUT_VOLTAGE_V,
				  &voltage) &&
	    huapsu_get_data_value(&huapsu_dri, HUAPSU_DATA_ID_OUT_CURRENT0_A,
				  &current)) {
		chgc_set_voltage_V(&chgc, voltage.value / 102.4f);
		chgc_set_current_A(&chgc, current.value / 102.4f);
		chgc_update(&chgc, delta_time_ms);
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
	ui.min_voltage_V = 300;
	ui.max_voltage_V = 400;
	ui.min_current_A = 0;
	ui.max_current_A = 40;
	psu_box_ui_update(&ui);

	setup_psuBOX();
	strcpy(psu_dev_info_str, "Device info is not available");
}

void loop(void)
{
	huapsu_val_entry voltage;
	huapsu_val_entry current;

	float current_limit = (ui.set_current_A / ui.max_current_A) *
			      (ui.charging_enabled ? 1.0f : 0.0f) * 1024.0f;

	if (huapsu_dri.online &&
	    huapsu_get_data_value(&huapsu_dri, HUAPSU_DATA_ID_OUT_VOLTAGE_V,
				  &voltage) &&
	    huapsu_get_data_value(&huapsu_dri, HUAPSU_DATA_ID_OUT_CURRENT0_A,
				  &current)) {
		ui.out_voltage_V = voltage.value / 1024.0;
		ui.out_current_A = current.value / 1024.0;
		ui.total_wh	 = chgc_get_energy_wh(&chgc);
	} else {
		ui.out_voltage_V = 0;
		ui.out_current_A = 0;
	}

	psu_box_ui_stats_update(&ui);
	psu_box_ui_info_update(&ui, psu_dev_info_str);

	lvgl_dri_update();

	huapsu_set_param_value(&huapsu_dri,
			       HUAPSU_PARAM_ID_ONLINE_OUTPUT_VOLTAGE_V,
			       ui.set_voltage_V * 1024.0f);
	huapsu_set_param_value(&huapsu_dri,
			       HUAPSU_PARAM_ID_OFFLINE_OUTPUT_VOLTAGE_V,
			       ui.set_voltage_V * 1024.0f);
	huapsu_set_param_value(
	    &huapsu_dri, HUAPSU_PARAM_ID_OVERVOLTAGE_PROTECTION_V, 400 * 1024);
	huapsu_set_param_value(&huapsu_dri,
			       HUAPSU_PARAM_ID_ONLINE_CURRENT_LIMIT_RATIO,
			       current_limit);
	huapsu_set_param_value(&huapsu_dri,
			       HUAPSU_PARAM_ID_OFFLINE_CURRENT_LIMIT_RATIO,
			       current_limit);

	loop_psuBOX();
}
