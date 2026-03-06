#include <assert.h>
#include <stdio.h>

#include "fan_ctl.h"

int main()
{
	uint16_t fan_duty = 0u;

	fan_duty = fan_control(30, 90);
	printf("PSU_FAN_CONTROL_PIN: %u\n", fan_duty);
	assert(fan_duty == 255u);

	fan_duty = fan_control(30, 100);
	printf("PSU_FAN_CONTROL_PIN: %u\n", fan_duty);
	assert(fan_duty == 255u);

	fan_duty = fan_control(30, 0);
	printf("PSU_FAN_CONTROL_PIN: %u\n", fan_duty);
	assert(fan_duty == 0u);

	fan_duty = fan_control(30, 44);
	printf("PSU_FAN_CONTROL_PIN: %u\n", fan_duty);
	assert(fan_duty == 0u);

	fan_duty = fan_control(30, 45);
	printf("PSU_FAN_CONTROL_PIN: %u\n", fan_duty);
	assert(fan_duty == 25u);

	fan_duty = fan_control(30, 46);
	printf("PSU_FAN_CONTROL_PIN: %u\n", fan_duty);
	assert(fan_duty > 25u);

	fan_duty = fan_control(-40, 46);
	printf("PSU_FAN_CONTROL_PIN: %u\n", fan_duty);
	assert(fan_duty > 25u);

	fan_duty = fan_control(-41, 46);
	printf("PSU_FAN_CONTROL_PIN: %u\n", fan_duty);
	assert(fan_duty == 255u);

	return 0;
}
