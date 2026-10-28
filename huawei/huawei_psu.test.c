#include <stdio.h>
#define HUAPSU_LOG(a) printf a
#include "huawei_psu.h"

#include <assert.h>
#include <stdint.h>

static void huapsu_print_params(struct huapsu *self)
{
	uint8_t iter;

	for (iter = 0; iter < HUAPSU_MAX_ENTRIES; iter++) {
		struct huapsu_val_entry entry;

		if (!huapsu_get_param_entry(self, iter, &entry)) {
			continue;
		}

		printf("%30s: %f %s\n", entry.meta.name,
		       (float)entry.value / (float)entry.bits_per_unit,
		       entry.meta.unit);
	}
}

static void huapsu_print_data(struct huapsu *self)
{
	uint8_t iter;

	for (iter = 0; iter < HUAPSU_MAX_ENTRIES; iter++) {
		struct huapsu_val_entry entry;

		if (!huapsu_get_data_value(self, iter, &entry)) {
			continue;
		}

		printf("%30s: %f %s\n", entry.meta.name,
		       (float)entry.value / (float)entry.bits_per_unit,
		       entry.meta.unit);
	}
}

static void huapsu_pop_params(struct huapsu *self, struct huapsu_frame *frame)
{
	/* 5 Params must be popped */
	assert(huapsu_pop_tx_frame(self, frame) == true);
	huapsu_print_frame(frame);

	assert(huapsu_pop_tx_frame(self, frame) == true);
	huapsu_print_frame(frame);

	assert(huapsu_pop_tx_frame(self, frame) == true);
	huapsu_print_frame(frame);

	assert(huapsu_pop_tx_frame(self, frame) == true);
	huapsu_print_frame(frame);

	assert(huapsu_pop_tx_frame(self, frame) == true);
	huapsu_print_frame(frame);
}

static void huapsu_pop_data(struct huapsu *self, struct huapsu_frame *frame)
{
	assert(huapsu_pop_tx_frame(self, frame) == true);
	huapsu_print_frame(frame);
}

static void huapsu_loop(struct huapsu *self)
{
	uint8_t iter;

	/* Bounded loop to prevent deadlock */
	for (iter = 0U; iter < 10U; iter++) {
		if (huapsu_step(self) == HUAPSU_EVENT_REQ_TIME) {
			huapsu_update_timers(self, 1);
		}

		if (huapsu_yield(self)) {
			break;
		}
	}
}

static void huapsu_test_autosend()
{
	struct huapsu_frame frame;
	struct huapsu	    psu;

	huapsu_init(&psu, HUAPSU_HW_VARIANT_R33640G1);

	assert(huapsu_set_param_value(
	    &psu, HUAPSU_PARAM_ID_ONLINE_OUTPUT_VOLTAGE_V, 350 * 1024));
	assert(huapsu_set_param_value(
	    &psu, HUAPSU_PARAM_ID_OFFLINE_OUTPUT_VOLTAGE_V, 350 * 1024));
	assert(huapsu_set_param_value(
	    &psu, HUAPSU_PARAM_ID_OVERVOLTAGE_PROTECTION_V, 400 * 1024));
	assert(huapsu_set_param_value(
	    &psu, HUAPSU_PARAM_ID_ONLINE_CURRENT_LIMIT_RATIO, 512));
	assert(huapsu_set_param_value(
	    &psu, HUAPSU_PARAM_ID_OFFLINE_CURRENT_LIMIT_RATIO, 512));

	huapsu_loop(&psu);

	huapsu_pop_params(&psu, &frame);
	huapsu_pop_data(&psu, &frame);

	assert(huapsu_pop_tx_frame(&psu, &frame) == false);

	/* Must fail on same value. */
	assert(!huapsu_set_param_value(
	    &psu, HUAPSU_PARAM_ID_OFFLINE_CURRENT_LIMIT_RATIO, 512));

	/* Must success on different value. */
	assert(huapsu_set_param_value(
	    &psu, HUAPSU_PARAM_ID_OFFLINE_CURRENT_LIMIT_RATIO, 513));

	huapsu_loop(&psu);
	printf("data_timer_r1ms: %u\n", psu.data_timer_r1ms);

	huapsu_loop(&psu);
	printf("data_timer_r1ms: %u\n", psu.data_timer_r1ms);
}

int main()
{
	uint8_t		    i;
	struct huapsu_frame frame;
	struct huapsu	    psu;

	huapsu_init(&psu, HUAPSU_HW_VARIANT_R33640G1);

	/* huapsu_pop_params(&psu, &frame); */
	huapsu_pop_data(&psu, &frame);
	assert(huapsu_pop_tx_frame(&psu, &frame) == false);

	printf("data_entries_recv_counter: %u\n",
	       psu.data_entries_recv_counter);
	assert(!huapsu_has_received_all_data(&psu));

	for (i = 0; i < 0xFFU; i++) {
		uint32_t test_val = (uint32_t)i * 1024;

		frame.id      = HUAPSU_MSG_ID_DATA_ANSW;
		frame.dlc     = 8U;
		frame.data[0] = HUAPSU_ANSW_CODE_OK;
		frame.data[1] = i;
		frame.data[4] = (uint8_t)((test_val >> 24U) & 0xFFU);
		frame.data[5] = (uint8_t)((test_val >> 16U) & 0xFFU);
		frame.data[6] = (uint8_t)((test_val >> 8U) & 0xFFU);
		frame.data[7] = (uint8_t)((test_val >> 0U) & 0xFFU);

		(void)huapsu_set_param_value(&psu, i, (int32_t)test_val);

		if (huapsu_parse_data_frame(&psu, &frame) == false &&
		    psu.err_code == HUAPSU_ERR_INVALID_DATA_ID) {
			/* printf("index out of range: %u\n", i); */
		} else {
			/* printf("index IN range:     %u\n", i); */
		}
	}

	printf("data_entries_recv_counter: %u\n",
	       psu.data_entries_recv_counter);
	assert(huapsu_has_received_all_data(&psu));

	huapsu_pop_params(&psu, &frame);
	assert(huapsu_pop_tx_frame(&psu, &frame) == false);

	huapsu_print_params(&psu);
	huapsu_print_data(&psu);

	huapsu_test_autosend();

	return 0;
}
