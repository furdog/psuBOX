#include "huawei_psu.h"

#include <stdint.h>
#include <stdio.h>

/**
 * Converts a scaled integer into a "int.frac" formatted string.
 *
 * @param value   The scaled integer (e.g., 12345 for 12.345)
 * @param divisor The fractional scalar/divisor (e.g., 1000 for 3 decimal
 * places)
 * @return        Pointer to a static buffer containing the formatted string.
 */
static const char *format_fractional_int(int32_t value, int32_t divisor)
{
	static char buffer[32];

	int32_t int_part;
	int32_t frac_part;

	int	width;
	int32_t temp;

	if (divisor <= 0) {
		(void)snprintf(buffer, sizeof(buffer), "ERR");
		return buffer;
	}

	int_part  = value / divisor;
	frac_part = value % divisor;

	/* Ensure the fractional part is always positive for formatting */
	if (frac_part < 0) {
		frac_part = -frac_part;
	}

	/* Determine the number of decimal digits from the divisor (e.g., 1000
	 -> 3 digits) */
	width = 0;
	temp  = divisor;
	while (temp > 1) {
		width++;
		frac_part *= 10;
		temp /= 10;
	}

	frac_part /= divisor;

	/* Edge case: If value is negative and int_part is 0, standard integer
	 division drops the negative sign (e.g., -50 / 100 = 0).
	 We explicitly prepend "-0" to handle this. */
	if (value < 0 && int_part == 0) {
		snprintf(buffer, sizeof(buffer), "-0.%0*d", width,
			 (int)frac_part);
	} else {
		snprintf(buffer, sizeof(buffer), "%d.%0*d", (int)int_part,
			 width, (int)frac_part);
	}

	return buffer;
}

static void huapsu_print_data(struct huapsu_data *self)
{
	printf("Input power:        %s W\n",
	       format_fractional_int(self->in_power_r10bpw, 1024U));
	printf("Input frequency:    %s Hz\n",
	       format_fractional_int(self->in_freq_r10bphz, 1024U));
	printf("Input current:      %s A\n",
	       format_fractional_int(self->in_current_r10bpa, 1024U));
	printf("Input voltage:      %s V\n",
	       format_fractional_int(self->in_voltage_r10bpv, 1024U));
	printf("Output power:       %s W\n",
	       format_fractional_int(self->out_power_r10bpw, 1024U));
	printf("Output voltage:     %s V\n",
	       format_fractional_int(self->out_voltage_r10bpv, 1024U));
	printf("Output current 0:   %s A\n",
	       format_fractional_int(self->out_current0_r10bpa, 1024U));
	printf("Output current 1:   %s A\n",
	       format_fractional_int(self->out_current1_r10bpa, 1024U));
	printf("Max output current: %s A\n",
	       format_fractional_int(self->out_current_max, 27U));
	printf("Output temperature: %s C\n",
	       format_fractional_int(self->out_temp_r10bpc, 1024U));

	/* For efficiency (10 bits per 100%), multiplying by 100 scales the
	 1024-based ratio into a readable percentage string (e.g., 95.50%). */
	printf(
	    "Efficiency:         %s %%\n",
	    format_fractional_int(self->efficiency_r10bp100pct * 100, 1024U));
}

int main()
{
	struct huapsu_frame frame;
	struct huapsu	    psu;

	huapsu_init(&psu, HUAPSU_HW_VARIANT_R33640G1);

	frame.id = HUAPSU_MSG_ID_DATA_ANSW;
	frame.dlc = 8U;
	frame.data[0] = HUAPSU_ANSW_CODE_OK;
	frame.data[1] = HUAPSU_DATA_ID_IN_POWER;
	frame.data[4] = 0x00U;
	frame.data[5] = 0x00U;
	frame.data[6] = 0x33U;
	frame.data[7] = 0xAAU;

	huapsu_parse_data_frame(&psu, &frame);

	huapsu_print_data(&psu.data);

	return 0;
}
