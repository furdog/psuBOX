# Huawei PSU

Huawei Rectfer Module driver (Hardware-Agnostic).
Suitable for various huawei PSU.
More model support may be added in future.

# Supported hardware

- R33640G1
- R4875G1

# Public API

```c
/** Initializes main instance. Hardware variant must be specified. */
void huapsu_init(struct huapsu *self, enum huapsu_hw_variant variant);

/** Gets specific param entry data struct (immutable).
    Returns true in case of success */
bool huapsu_get_param_entry(struct huapsu *self, uint8_t param_id,
			    struct huapsu_val_entry *entry);

/** Set specific param value.
    Returns true in case of success */
bool huapsu_set_param_value(struct huapsu *self, uint8_t param_id,
			    int32_t value);

/** Gets specific param entry data struct (immutable).
    Returns true in case of success */
bool huapsu_get_data_value(struct huapsu *self, uint8_t data_id,
			   struct huapsu_val_entry *entry);

/** Returns true if all data fields has been received. */
bool huapsu_has_received_all_data(struct huapsu *self);

/** Main fsm routine. Must be called in while loop until busy flag
 *  is set to true. */
uint8_t huapsu_step(struct huapsu *self);

/** FSM is busy and shall not be interrupted by anything for safety reasons. */
bool huapsu_is_busy(struct huapsu *self);

/** FSM is ready to be interrupted. (inverse of huapsu_is_busy) */
bool huapsu_yield(struct huapsu *self);

/** Push RX frame into driver.
 *  Returns true on success and false if frame is already queued. */
bool huapsu_push_rx_frame(struct huapsu		    *self,
			  const struct huapsu_frame *frame);

/** Pop TX frame from the driver.
 *  Returns false if there are no frames left to pop. */
bool huapsu_pop_tx_frame(struct huapsu *self, struct huapsu_frame *frame);

/** Updates all internal timers. Should only be called on
    HUAPSU_EVENT_REQ_TIME event */
void huapsu_update_timers(struct huapsu *self, uint32_t delta_time_ms);
```

# Example

C pseudocode:

```c
struct huapsu psu; /* Define main instance */

...

/* Call init, provide variant */
huapsu_init(&psu, HUAPSU_HW_VARIANT_R33640G1);

/* Set output voltage (multiply by 1024) */
assert(huapsu_set_param_value(
       &psu, HUAPSU_PARAM_ID_ONLINE_OUTPUT_VOLTAGE_V, 350 * 1024));

/* Fallback value (in case if can communication fails at some point) */
assert(huapsu_set_param_value(
       &psu, HUAPSU_PARAM_ID_OFFLINE_OUTPUT_VOLTAGE_V, 350 * 1024));

assert(huapsu_set_param_value(
       &psu, HUAPSU_PARAM_ID_OVERVOLTAGE_PROTECTION_V, 400 * 1024));

/* Current limit 512/1024 == 50% of maximum PSU current */
assert(huapsu_set_param_value(
       &psu, HUAPSU_PARAM_ID_ONLINE_CURRENT_LIMIT_RATIO, 512));
assert(huapsu_set_param_value(
       &psu, HUAPSU_PARAM_ID_OFFLINE_CURRENT_LIMIT_RATIO, 512));

...

/* Bounded loop to prevent deadlock. */
for (iter = 0U; iter < 10U; iter++) {
	/* Call update_timers and provide delta time from previous cycle
	 * Only done on request. */
	if (huapsu_step(&psu) == HUAPSU_EVENT_REQ_TIME) {
		huapsu_update_timers(&psu, delta_time_ms);
	}

	/* Always exit when not busy (basically yield) 
	 * NOTE: This should always be called AFTER huapsu_step */
	if (huapsu_yield(&psu)) {
		break;
	}
}

...

/** CAN frame (virtual) */
struct huapsu_frame frame;

if (huapsu_pop_tx_frame(&psu, &frame) == true) {
	/* Invoke your hardware HAL at this point to SEND this frame */
	HAL_CAN_Transmit(&frame, ...);

	/* ... */
}

...

/* Invoke your hardware HAL at this point to RECEIVE this frame */
/* ... */
if (HAL_CAN_Receive(&frame, ...) == OK) {
	/* Push RX frame into fsm.
	 * Should only be done once before full fsm update.
	 * HUAPSU is unable to receive multiple frames at once. */
	(void)huapsu_push_rx_frame(&psu, &frame);
}
```

# Problems
- Unfinished API. Subject to change.
- User is forced to manage raw values.
- No proper diagnostics
- No proper test coverage

# Implementation notes
> 27.10.2026

- Created readme. Basically first test release.
- Added ONLINE/OFFLINE events.
