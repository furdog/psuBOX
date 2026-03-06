#include <stdint.h>

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

/** Simple, integer based fan PWM contoller
 * ==45C:10% , >=90C:100%
 */
uint16_t fan_control(int16_t t0, int16_t t1)
{
	int16_t min_temp = MIN(t0, t1);
	int16_t max_temp = MAX(t0, t1);

	uint16_t pwm_duty = 0u;

	uint16_t min_duty   = 25;		   /* 10% */
	uint16_t max_duty   = 255u;		   /* 100% */
	uint16_t duty_range = max_duty - min_duty; /* 90% */

	/* Clamp max_temp */
	if (max_temp > 90) {
		max_temp = 90;
	}

	if (max_temp >= 45) {
		/* Calculate intensity scalar */
		uint16_t intensity     = ((uint16_t)max_temp - 45u);
		uint16_t max_intensity = 90u - 45u;

		/* 10% + (90% * intensity) / max_intensity */
		pwm_duty = min_duty + (duty_range * intensity) / max_intensity;
	}

	if (min_temp < -40) {
		pwm_duty = max_duty;
	}

	return pwm_duty;
}
