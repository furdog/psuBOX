#include "huawei_psu.h"

#include <stdint.h>
#include <stdio.h>

static void huapsu_print_data(struct huapsu *self)
{
	uint8_t iter;

	for (iter = 0; iter < (uint8_t)HUAPSU_DATA_ID_COUNT; iter++) {
		struct huapsu_val_entry *entry = &self->data_entries[iter];
		printf("%30s: %f %s\n", entry->meta.name,
		       (float)entry->value / (float)entry->bits_per_unit,
		       entry->meta.unit);
	}
}

static void huapsu_print_params(struct huapsu *self)
{
	uint8_t iter;

	for (iter = 0; iter < (uint8_t)HUAPSU_PARAM_ID_COUNT; iter++) {
		struct huapsu_val_entry *entry = &self->param_entries[iter];
		printf("%30s: %f %s\n", entry->meta.name,
		       (float)entry->value / (float)entry->bits_per_unit,
		       entry->meta.unit);
	}
}

int main()
{
	uint8_t		    i;
	struct huapsu_frame frame;
	struct huapsu	    psu;

	huapsu_init(&psu, HUAPSU_HW_VARIANT_R33640G1);

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

		huapsu_set_param(huapsu_get_param_entry(&psu, i), test_val);

		if (huapsu_parse_data_frame(&psu, &frame) == false &&
		    psu.err_code == HUAPSU_ERR_INVALID_DATA_ID) {
			/* printf("index out of range: %u\n", i); */
		} else {
			/* printf("index IN range:     %u\n", i); */
		}
	}

	huapsu_print_params(&psu);
	huapsu_print_data(&psu);

	return 0;
}
