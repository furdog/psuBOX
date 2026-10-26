/**
 * @file huawei_psu.h
 * @brief Huawei Rectfer Module driver (Hardware-Agnostic)
 *
 * Protocol CAN2.0
 * Bitrate 125kbps
 * Extended frames used
 *
 * Supported devices:
 * - R33640G1 max: 400V, 37A (40A ?)
 * - R4875G1  max: 58V,  75A
 *
 * ```LICENSE
 * Copyright (c) 2026 furdog <https://github.com/furdog>
 *
 * SPDX-License-Identifier: 0BSD
 * ```
 *
 * Be free, be wise and take care of yourself!
 * With best wishes and respect, furdog
 */

#ifndef HUAPSU_H
#define HUAPSU_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/******************************************************************************
 * GLOBALS DEFINITIONS
 *****************************************************************************/
#define HUAPSU_FRAME_MAX_LEN 8U
#define HUAPSU_MAX_TX_FRAMES 8U
#define HUAPSU_DATA_REQUEST_INTERVAL_MS 100U
#define HUAPSU_PARAM_ID_SET_INTERVAL_MS 1000U
#define HUAPSU_MAX_ENTRIES 0xFFU

/******************************************************************************
 * HARDWARE INTERFACE (agnostic)
 *****************************************************************************/
/** Generic CAN2.0 frame representation. */
struct huapsu_frame {
	uint32_t id;
	int8_t	 dlc;
	uint8_t	 data[HUAPSU_FRAME_MAX_LEN];
};

/******************************************************************************
 * PROTOCOL SPECIFIC
 *****************************************************************************/
enum huapsu_msg_id {
	HUAPSU_MSG_ID_DATA_REQU = 0x108040FE,
	HUAPSU_MSG_ID_DATA_ANSW = 0x1081407F,
	HUAPSU_MSG_ID_PARAM_SET = 0x108180FE,
	HUAPSU_MSG_ID_PARAM_ACK = 0x1081807E
};

enum huapsu_answ_code {
	HUAPSU_ANSW_CODE_OK  = 0x01U,
	HUAPSU_ANSW_CODE_ERR = 0x21U
};

/******************************************************************************
 * PARAMS & DATA COMMON
 *****************************************************************************/
struct huapsu_id_range {
	uint8_t start;
	uint8_t end;
};

struct huapsu_meta {
	char *name;
	char *unit;
};

struct huapsu_val_entry {
	struct huapsu_meta meta;

	int32_t	 value;
	uint16_t bits_per_unit;
	bool	 dirty;
};

/******************************************************************************
 * PARAMS (Write-only fields).
 *****************************************************************************/
enum huapsu_param_id {
	HUAPSU_PARAM_ID_ONLINE_OUTPUT_VOLTAGE_V	    = 0U,
	HUAPSU_PARAM_ID_OFFLINE_OUTPUT_VOLTAGE_V    = 1U,
	HUAPSU_PARAM_ID_OVERVOLTAGE_PROTECTION_V    = 2U,
	HUAPSU_PARAM_ID_ONLINE_CURRENT_LIMIT_RATIO  = 3U,
	HUAPSU_PARAM_ID_OFFLINE_CURRENT_LIMIT_RATIO = 4U,

	HUAPSU_PARAM_ID_COUNT	= 5U,
	HUAPSU_PARAM_ID_INVALID = 0xFFU
};

extern const struct huapsu_meta g_huapsu_param_meta[HUAPSU_PARAM_ID_COUNT];

#ifndef HUAPSU_NO_IMPL
/** huapsu_param_id metadata array (Values 0 to 4). */
const struct huapsu_meta g_huapsu_param_meta[HUAPSU_PARAM_ID_COUNT] = {
    {	   "Online output voltage", "V"},
    {	  "Offline output voltage", "V"},
    {	  "Overvoltage protection", "V"},
    { "Online current limit ratio",  ""},
    {"Offline current limit ratio",  ""}
};

/** Maps param ID into continuous, flat range without gaps.
 *  Starts from 0. Returns 0xFFU in case of bad vid */
uint8_t huapsu_map_param_id_to_slot(uint8_t vid)
{
	const uint8_t invalid_id  = 0xFFU;
	const uint8_t range_start = 0x00U;
	const uint8_t range_end	  = 0x04U;

	uint8_t result = invalid_id;

	if ((vid >= range_start) && (vid <= range_end)) {
		result = vid - range_start;
	}

	return result;
}

/** Map slot number into param id (same as above, but reversed). */
uint8_t huapsu_map_slot_to_param_id(uint8_t vid)
{
	const uint8_t invalid_id  = 0xFFU;
	const uint8_t range_start = 0x00U;
	const uint8_t range_end	  = 0x04U;

	uint8_t result = invalid_id;

	if ((vid >= range_start) && (vid <= range_end)) {
		result = vid + range_start;
	}

	return result;
}
#endif /* HUAPSU_NO_IMPL */

/******************************************************************************
 * DATA (Read-only fields).
 *****************************************************************************/
enum huapsu_data_id {
	HUAPSU_DATA_ID_IN_POWER_W	       = 0x70U,
	HUAPSU_DATA_ID_IN_FREQ_HZ	       = 0x71U,
	HUAPSU_DATA_ID_IN_CURRENT_A	       = 0x72U,
	HUAPSU_DATA_ID_OUT_POWER_W	       = 0x73U,
	HUAPSU_DATA_ID_EFFICIENCY_W	       = 0x74U,
	HUAPSU_DATA_ID_OUT_VOLTAGE_V	       = 0x75U,
	HUAPSU_DATA_ID_OUT_CURRENT_LIMIT_RATIO = 0x76U,
	HUAPSU_DATA_ID_IN_VOLTAGE_V	       = 0x78U,
	HUAPSU_DATA_ID_OUT_TEMP_C	       = 0x7FU,
	HUAPSU_DATA_ID_IN_TEMP_C	       = 0x80U,
	HUAPSU_DATA_ID_OUT_CURRENT0_A	       = 0x81U,
	HUAPSU_DATA_ID_OUT_CURRENT1_A	       = 0x82U,
	HUAPSU_DATA_ID_UNKNOWN		       = 0x83U,

	HUAPSU_DATA_ID_COUNT   = 13U,
	HUAPSU_DATA_ID_INVALID = 0xFFU
};

extern const struct huapsu_meta g_huapsu_data_meta[HUAPSU_DATA_ID_COUNT];

#ifndef HUAPSU_NO_IMPL
/** huapsu_data_id metadata array (Indexed from 0x70U to 0x83U, with gaps
 * filled). */
const struct huapsu_meta g_huapsu_data_meta[HUAPSU_DATA_ID_COUNT] = {
    {		    "Input power",  "W"}, /* 0x70 */
    {		"Input frequency", "Hz"}, /* 0x71 */
    {		  "Input current",  "A"}, /* 0x72 */
    {		   "Output power",  "W"}, /* 0x73 */
    {		     "Efficiency",  "%"}, /* 0x74 */
    {		 "Output voltage",  "V"}, /* 0x75 */
    {"Output current limit ratio",   ""}, /* 0x76 */
    {		  "Input voltage",  "V"}, /* 0x78 */
    {	     "Output temperature",  "C"}, /* 0x7F */
    {	      "Input temperature",  "C"}, /* 0x80 */
    {	       "Output current 0",  "A"}, /* 0x81 */
    {	       "Output current 1",  "A"}, /* 0x82 */
    {			"Unknown",   ""}  /* 0x83 */
};

/** Maps data ID into continuous, flat range without gaps.
 *  Starts from 0. Returns 0xFFU in case of bad vid */
uint8_t huapsu_map_data_id_to_slot(uint8_t vid)
{
	const uint8_t invalid_id = 0xFFU;

	const struct huapsu_id_range ranges[3U] = {
	    {0x70U, 0x76U},
	       {0x78U, 0x78U},
		  {0x7FU, 0x83U}
	};

	const struct huapsu_id_range *range = NULL;

	uint8_t result = invalid_id;
	uint8_t iter;
	uint8_t idx = 0U; /**< Flat array index. */

	/* Check if it falls into any range. */
	for (iter = 0U; iter < 3U; iter++) {
		if ((vid >= ranges[iter].start) && (vid <= ranges[iter].end)) {
			range = &ranges[iter];
			break;
		}

		idx += ranges[iter].end - ranges[iter].start + 1U;
	}

	/** We've found valid range */
	if (range != NULL) {
		result = idx + vid - range->start;
	}

	return result;
}
#endif /* HUAPSU_NO_IMPL */

/******************************************************************************
 * IMPLEMENTATION SPECIFIC ENUMS
 *****************************************************************************/
enum huapsu_hw_variant {
	HUAPSU_HW_VARIANT_NONE, /** Up to user. */
	HUAPSU_HW_VARIANT_R33640G1,
	HUAPSU_HW_VARIANT_R4875G1
};

enum huapsu_state {
	HUAPSU_STATE_YIELD, /**< Give other tasks control. */
	HUAPSU_STATE_RX,    /**< Receiving messages. */
	HUAPSU_STATE_TX,    /**< Sending messages. */
	HUAPSU_STATE_TIMERS /**< Updating timers. */
};

enum huapsu_err {
	HUAPSU_ERR_NONE,
	HUAPSU_ERR_INVALID_ANSWER,
	HUAPSU_ERR_INVALID_ID,
	HUAPSU_ERR_INVALID_DLC,
	HUAPSU_ERR_INVALID_DATA_ID
};

enum huapsu_event {
	HUAPSU_EVENT_NONE,     /**< No events emited. */
	HUAPSU_EVENT_REQ_TIME, /**< Timers update requested. */
	HUAPSU_EVENT_MESSAGE,  /**< New message arrived. */
	HUAPSU_EVENT_ERROR     /**< Some error has occured. */
};

/******************************************************************************
 * DEVICE CONFIGURATION
 *****************************************************************************/
struct huapsu_config {
	uint32_t min_output_voltage_r10bpv; /**< 10 bit per Volt unit. */
	uint32_t max_output_voltage_r10bpv; /**< 10 bit per Volt unit. */
	uint32_t max_output_current_r10bpa; /**< 10 bit per Amp unit. */
	uint32_t max_output_power_r10bpw;   /**< 10 bit per Watt unit. */
};

extern const struct huapsu_config g_huapsu_config_r33640g1;
extern const struct huapsu_config g_huapsu_config_r4875g1;

/******************************************************************************
 * MAIN INSTANCE
 *****************************************************************************/
struct huapsu {
	uint8_t state;

	struct huapsu_config cfg;

	struct huapsu_val_entry param_entries[HUAPSU_PARAM_ID_COUNT];
	struct huapsu_val_entry data_entries[HUAPSU_DATA_ID_COUNT];

	struct huapsu_frame rx_frame;

	/** Param entries queued for send.
	 *  The value stores index to value in param_entries[] */
	uint8_t param_tx_queue[HUAPSU_PARAM_ID_COUNT];
	uint8_t param_tx_count;

	bool online; /**< PSU is reported to be online. */
	bool busy;   /**< Should not yield. */

	/* Timers */
	/** How long are we allowed to stay offline. */
	uint32_t offline_timeout_r1ms;
	uint32_t offline_timer_r1ms; /**< How long we've been offline. */
	uint32_t data_timer_r1ms;  /**< Time passed since last data request. */
	uint32_t param_timer_r1ms; /**< Time passed since last param update. */

	/* Diagnostics */
	uint8_t err_code;
	uint8_t err_state;
	uint8_t last_msg_id;
	uint8_t last_answ_code;
};

/** Initializes main instance. Hardware variant must be specified. */
void huapsu_init(struct huapsu *self, enum huapsu_hw_variant variant);

#ifndef HUAPSU_NO_IMPL
const struct huapsu_config g_huapsu_config_r33640g1 = {
    300U * 1024U, 400U * 1024U, 37U * 1024U, 15000U * 1024U};

const struct huapsu_config g_huapsu_config_r4875g1 = {
    42U * 1024U, 58U * 1024U, 75U * 1024U, 15000U * 1024U};

void huapsu_init(struct huapsu *self, const enum huapsu_hw_variant variant)
{
	uint8_t	       iter;
	const uint16_t default_resolution = 1024U;

	const struct huapsu _self = {0};
	*self			  = _self;

	switch (variant) {
	case HUAPSU_HW_VARIANT_R33640G1:
		self->cfg = g_huapsu_config_r33640g1;
		break;

	case HUAPSU_HW_VARIANT_R4875G1:
		self->cfg = g_huapsu_config_r4875g1;
		break;

	default:
		break;
	}

	self->rx_frame.dlc = -1; /* Indicates that there is no frame. */

	/* Set timers to their max interval, so they fire immediately. */
	self->data_timer_r1ms  = HUAPSU_DATA_REQUEST_INTERVAL_MS;
	self->param_timer_r1ms = HUAPSU_PARAM_ID_SET_INTERVAL_MS;

	/* Set common resolution for params */
	for (iter = 0; iter < (uint8_t)HUAPSU_PARAM_ID_COUNT; iter++) {
		/* Common for most values. */
		self->param_entries[iter].meta = g_huapsu_param_meta[iter];
		self->param_entries[iter].bits_per_unit = default_resolution;
	}

	for (iter = 0; iter < (uint8_t)HUAPSU_DATA_ID_COUNT; iter++) {
		/** Common for most values. */
		self->data_entries[iter].meta = g_huapsu_data_meta[iter];
		self->data_entries[iter].bits_per_unit = default_resolution;
	}
}
#endif /* HUAPSU_NO_IMPL */

/******************************************************************************
 * GETTERS / SETTERS
 *****************************************************************************/
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

#ifndef HUAPSU_NO_IMPL
bool huapsu_get_param_entry(struct huapsu *self, const uint8_t param_id,
			    struct huapsu_val_entry *entry)
{
	bool	success = false;
	uint8_t slot	= huapsu_map_param_id_to_slot(param_id);

	if (slot != (uint8_t)-1) {
		*entry	= self->param_entries[slot];
		success = true;
	}

	return success;
}

bool huapsu_set_param_value(
    struct huapsu *self,
    const uint8_t  param_id, /* NOLINT(bugprone-easily-swappable-parameters) */
    const int32_t  value)
{
	bool	success = false;
	uint8_t slot	= huapsu_map_param_id_to_slot(param_id);

	if (slot != (uint8_t)-1) {
		struct huapsu_val_entry *entry = &self->param_entries[slot];

		/* Add this to param to the TX queue.
		 * This can be done only once. Until frame is popped.
		 */
		if (!entry->dirty) {
			self->param_tx_queue[self->param_tx_count] = slot;
			self->param_tx_count++;
		}

		entry->dirty = true;
		entry->value = value;
		success	     = true;
	}

	return success;
}

bool huapsu_get_data_value(struct huapsu *self, const uint8_t data_id,
			   struct huapsu_val_entry *entry)
{
	bool	success = false;
	uint8_t slot	= huapsu_map_data_id_to_slot(data_id);

	if (slot != (uint8_t)-1) {
		*entry	= self->data_entries[slot];
		success = true;
	}

	return success;
}
#endif /* HUAPSU_NO_IMPL */

/******************************************************************************
 * FINITE STATE MACHINE IMPLEMENTATION
 *****************************************************************************/
/** Main fsm routine. Must be called in while loop until busy flag
 *  is set to true. */
uint8_t huapsu_step(struct huapsu *self);

/** FSM is busy and shall not be interrupted by anything for safety reasons. */
bool huapsu_is_busy(struct huapsu *self);

#ifndef HUAPSU_NO_IMPL
void huapsu_record_error(struct huapsu *self, const uint8_t err_code)
{
	self->err_code	= err_code;
	self->err_state = self->state;
}

int32_t huapsu_decode_value(const struct huapsu_frame *frame)
{
	/** Value starts at 5th byte and has 32bit length.
	 *  Big-endian order (MSB goes first). */
	const uint32_t raw = ((uint32_t)frame->data[4U] << 24U) |
			     (uint32_t)frame->data[5U] << 16U |
			     (uint32_t)frame->data[6U] << 8U |
			     (uint32_t)frame->data[7U] << 0U;

	/** All values are signed integers. */
	const int32_t value = (int32_t)raw;

	return value;
}

void huapsu_encode_value(struct huapsu_frame *frame, const int32_t value)
{
	/* Offset within frame data */
	const uint8_t ofs = 4U;

	/* Encoded value */
	const uint32_t enc0 = ((uint32_t)value >> 24U) & 0xFFU;
	const uint32_t enc1 = ((uint32_t)value >> 16U) & 0xFFU;
	const uint32_t enc2 = ((uint32_t)value >> 8U) & 0xFFU;
	const uint32_t enc3 = ((uint32_t)value >> 0U) & 0xFFU;

	/* Put it into frame data */
	frame->data[ofs + 0U] = enc0;
	frame->data[ofs + 1U] = enc1;
	frame->data[ofs + 2U] = enc2;
	frame->data[ofs + 3U] = enc3;
}

bool huapsu_parse_param_frame(struct huapsu		*self,
			      const struct huapsu_frame *frame)
{
	bool success = false;

	uint8_t param_id = frame->data[1U];
	uint8_t slot	 = 0U;

	const int32_t value = huapsu_decode_value(frame);

	self->last_answ_code = frame->data[0U];
	self->last_msg_id    = frame->data[1U];

	if (self->last_answ_code == (uint8_t)HUAPSU_ANSW_CODE_OK) {
		success = true;

		slot = huapsu_map_param_id_to_slot(param_id);
		if (slot != (uint8_t)-1) {
			self->param_entries[slot].value = value;
		} else {
			huapsu_record_error(self, HUAPSU_ERR_INVALID_DATA_ID);
		}
	} else {
		huapsu_record_error(self, HUAPSU_ERR_INVALID_ANSWER);
	}

	return success;
}

bool huapsu_parse_data_frame(struct huapsu	       *self,
			     const struct huapsu_frame *frame)
{
	bool success = false;

	uint8_t data_id = frame->data[1U];
	uint8_t slot	= 0U;

	self->last_answ_code = frame->data[0U];
	self->last_msg_id    = frame->data[1U];

	if (self->last_answ_code == (uint8_t)HUAPSU_ANSW_CODE_OK) {
		success = true;

		slot = huapsu_map_data_id_to_slot(data_id);
		if (slot != (uint8_t)-1) {
			self->data_entries[slot].value =
			    huapsu_decode_value(frame);

			self->data_entries[slot].dirty = true;
		} else {
			huapsu_record_error(self, HUAPSU_ERR_INVALID_DATA_ID);
			success = false;
		}
	} else {
		huapsu_record_error(self, HUAPSU_ERR_INVALID_ANSWER);
	}

	return success;
}

bool huapsu_parse_input_frame(struct huapsu		*self,
			      const struct huapsu_frame *frame)
{
	bool success = false;

	switch (frame->id) {
	case HUAPSU_MSG_ID_DATA_ANSW:
		if (frame->dlc == (int8_t)HUAPSU_FRAME_MAX_LEN) {
			success = huapsu_parse_data_frame(self, frame);
		} else {
			huapsu_record_error(self, HUAPSU_ERR_INVALID_DLC);
		}
		break;

	case HUAPSU_MSG_ID_PARAM_ACK:
		if (frame->dlc == (int8_t)HUAPSU_FRAME_MAX_LEN) {
			success = huapsu_parse_param_frame(self, frame);
		} else {
			huapsu_record_error(self, HUAPSU_ERR_INVALID_DLC);
		}
		break;

	default:
		huapsu_record_error(self, HUAPSU_ERR_INVALID_ID);
		break;
	}

	return success;
}

uint8_t huapsu_state_yield(struct huapsu *self)
{
	self->state = HUAPSU_STATE_RX;
	self->busy  = true;
	return HUAPSU_EVENT_NONE;
}

uint8_t huapsu_state_rx(struct huapsu *self)
{
	enum huapsu_event event = HUAPSU_EVENT_NONE;

	if (self->rx_frame.dlc >= 0) {
		if (!huapsu_parse_input_frame(self, &self->rx_frame)) {
			event = HUAPSU_EVENT_ERROR;
		} else {
			self->online		 = true;
			self->offline_timer_r1ms = 0U;
		}

		/* Invalidate processed frame. */
		self->rx_frame.dlc = -1;
	}

	/* Advance to the next state. */
	self->state = HUAPSU_STATE_TX;

	return event;
}

uint8_t huapsu_state_tx(struct huapsu *self)
{
	enum huapsu_event event = HUAPSU_EVENT_NONE;

	self->state = HUAPSU_STATE_TIMERS;

	return event;
}

uint8_t huapsu_state_timers(struct huapsu *self)
{
	self->state = HUAPSU_STATE_YIELD;
	return HUAPSU_EVENT_REQ_TIME;
}

uint8_t huapsu_step(struct huapsu *self)
{
	/* By default we yield, unless more things left to do */
	enum huapsu_event event = HUAPSU_EVENT_NONE;

	/* Clear all errors before update */
	self->err_code = HUAPSU_ERR_NONE;

	switch (self->state) {
	case HUAPSU_STATE_YIELD:
		event = huapsu_state_yield(self);
		break;

	case HUAPSU_STATE_RX:
		event = huapsu_state_rx(self);
		break;

	case HUAPSU_STATE_TX:
		event = huapsu_state_tx(self);
		break;

	case HUAPSU_STATE_TIMERS:
		event = huapsu_state_timers(self);
		break;

	default:
		break;
	}

	return event;
}

bool huapsu_is_busy(struct huapsu *self) { return self->busy; }
#endif /* HUAPSU_NO_IMPL */

/******************************************************************************
 * INPUT / OUTPUT
 *****************************************************************************/
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

#ifndef HUAPSU_NO_IMPL
bool huapsu_push_rx_frame(struct huapsu		    *self,
			  const struct huapsu_frame *frame)
{
	bool success = false;

	if (self->rx_frame.dlc < 0) {
		self->rx_frame = *frame;
		success	       = true;
	}

	return success;
}

bool huapsu_pop_tx_frame(struct huapsu *self, struct huapsu_frame *frame)
{
	bool success = false;

	uint8_t iter;

	/* Param values must be sent every interval automatically. */
	if (self->param_timer_r1ms >= HUAPSU_PARAM_ID_SET_INTERVAL_MS) {
		self->param_timer_r1ms = 0U;

		self->param_tx_count = 0U;

		for (iter = 0U; iter < (uint8_t)HUAPSU_PARAM_ID_COUNT;
		     iter++) {
			/* Do not queue uninitialized frames. */
			if (!self->param_entries[iter].dirty) {
				continue;
			}

			self->param_tx_queue[self->param_tx_count] = iter;
			self->param_tx_count++;
		}
	}

	/* Only send params when online? TODO decide if that's necessary */
	if ((self->param_tx_count > 0U) /*&& self->online*/) {
		const uint8_t slot =
		    self->param_tx_queue[self->param_tx_count - 1U];
		const uint8_t pid = huapsu_map_slot_to_param_id(slot);

		struct huapsu_val_entry *entry = NULL;

		/* TODO this should not happen at all */
		if (pid != (uint8_t)-1) {
			entry = &self->param_entries[slot];

			frame->id      = HUAPSU_MSG_ID_PARAM_SET;
			frame->dlc     = HUAPSU_FRAME_MAX_LEN;
			frame->data[0] = 0x01U;
			frame->data[1] = pid;
			frame->data[2] = 0x00U;
			frame->data[3] = 0x00U;

			huapsu_encode_value(frame, entry->value);
		}

		self->param_tx_count--;
		success = true;
	} else if (self->data_timer_r1ms >= HUAPSU_DATA_REQUEST_INTERVAL_MS) {
		/* Data values must be requested every interval automatically.
		 */
		self->data_timer_r1ms = 0U;

		frame->id  = HUAPSU_MSG_ID_DATA_REQU;
		frame->dlc = HUAPSU_FRAME_MAX_LEN;

		for (iter = 0U; iter < HUAPSU_FRAME_MAX_LEN; iter++) {
			frame->data[iter] = 0x00U;
		}
		success = true;
	} else {
	}

	return success;
}

void huapsu_update_timers(struct huapsu *self, const uint32_t delta_time_ms)
{
	if (self->state != (uint8_t)HUAPSU_STATE_TIMERS) {
		/* Invalid state. TODO exception */
	} else {
		/* Increment offline timer, until it reached max
		 * threshold */
		if (self->offline_timer_r1ms < self->offline_timeout_r1ms) {
			self->offline_timer_r1ms += delta_time_ms;
		} else {
			self->online = false;
		}

		if (self->param_timer_r1ms < HUAPSU_PARAM_ID_SET_INTERVAL_MS) {
			self->param_timer_r1ms += delta_time_ms;
		}

		if (self->data_timer_r1ms < HUAPSU_DATA_REQUEST_INTERVAL_MS) {
			self->data_timer_r1ms += delta_time_ms;
		}
	}
}
#endif /* HUAPSU_NO_IMPL */

#endif /* HUAPSU_H */
