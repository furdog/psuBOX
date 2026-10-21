/**
 * @file bite.h
 * @brief Huawei Rectfer Module driver (Hardware-Agnostic)
 *
 * Protocol CAN2.0
 * Bitrate 125kbps
 * Extended frames used
 *
 * Supported devices:
 * - R33640G1
 *
 * ```LICENSE
 * Copyright (c) 2025 furdog <https://github.com/furdog>
 *
 * SPDX-License-Identifier: 0BSD
 * ```
 *
 * Be free, be wise and take care of yourself!
 * With best wishes and respect, furdog
 */

#ifndef HUAPSU_H
#define HUAPSU_H

#include <stdbool.h>
#include <stdint.h>

/** Default current calibration multiplier. */
#define HUAPSU_DEF_CUR_CAL_MUL 27U
#define HUAPSU_FRAME_MAX_LEN 8U

enum huapsu_hw_variant {
	HUAPSU_HW_VARIANT_R33640G1,
	HUAPSU_HW_VARIANT_R4875G1
};

enum huapsu_state {
	HUAPSU_STATE_YIELD, /**< Give other tasks control. */
	HUAPSU_STATE_RX,    /**< Receiving messages. */
	HUAPSU_STATE_TX,    /**< Sending messages. */
	HUAPSU_STATE_TIMERS /**< Updating timers. */
};

enum huapsu_answ_code {
	HUAPSU_ANSW_CODE_OK  = 0x01U,
	HUAPSU_ANSW_CODE_ERR = 0x21U
};

enum huapsu_err_code {
	HUAPSU_ERR_NONE,
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

enum huapsu_msg_id {
	HUAPSU_MSG_ID_DATA_REQU = 0x108040FE,
	HUAPSU_MSG_ID_DATA_ANSW = 0x1081407F,
	HUAPSU_MSG_ID_PARAM_SET = 0x108180FE,
	HUAPSU_MSG_ID_PARAM_ACK = 0x1081807E
};

enum huapsu_data_id {
	HUAPSU_DATA_ID_RANGE_START = 0x70U,
	HUAPSU_DATA_ID_RANGE_END   = 0x83U,

	HUAPSU_DATA_ID_IN_POWER	       = 0x70U,
	HUAPSU_DATA_ID_IN_FREQ	       = 0x71U,
	HUAPSU_DATA_ID_IN_CURRENT      = 0x72U,
	HUAPSU_DATA_ID_OUT_POWER       = 0x73U,
	HUAPSU_DATA_ID_EFFICIENCY      = 0x74U,
	HUAPSU_DATA_ID_OUT_VOLTAGE     = 0x75U,
	HUAPSU_DATA_ID_OUT_CURRENT_MAX = 0x76U,
	HUAPSU_DATA_ID_IN_VOLTAGE      = 0x78U,
	HUAPSU_DATA_ID_OUT_TEMP	       = 0x7FU,
	HUAPSU_DATA_ID_OUT_CURRENT0    = 0x81U,
	HUAPSU_DATA_ID_OUT_CURRENT1    = 0x82U,
	HUAPSU_DATA_ID_UNKNOWN	       = 0x83U,
	HUAPSU_DATA_ID_INVALID	       = 0xFFU
};

enum huapsu_param {
	HUAPSU_PARAM_ONLINE_OUTPUT_VOLTAGE  = 0U,
	HUAPSU_PARAM_OFFLINE_OUTPUT_VOLTAGE = 1U,
	HUAPSU_PARAM_OVERVOLTAGE_PROTECTION = 2U,
	HUAPSU_PARAM_ONLINE_CURRENT_LIMIT   = 3U,
	HUAPSU_PARAM_OFFLINE_CURRENT_LIMIT  = 4U
};

/** Generic CAN2.0 frame representation. */
struct huapsu_frame {
	uint32_t id;
	int8_t	 dlc;
	uint8_t	 data[HUAPSU_FRAME_MAX_LEN];
};

/** Raw data. */
struct huapsu_data {
	int32_t in_power_r10bpw;	/**< 10 bit per Watt unit. */
	int32_t in_freq_r10bphz;	/**< 10 bit per Hz unit. */
	int32_t in_current_r10bpa;	/**< 10 bit per Amp unit. */
	int32_t out_power_r10bpw;	/**< 10 bit per Watt unit. */
	int32_t efficiency_r10bp100pct; /**< 10 bit per 100 Percents. */
	int32_t out_voltage_r10bpv;	/**< 10 bit per Volt unit. */
	int32_t out_current_max;	/**< Special (scaled) unit. */
	int32_t in_voltage_r10bpv;	/**< 10 bit per Volt unit. */
	int32_t out_temp_r10bpc;	/**< 10 bit per Celsius unit. */
	int32_t out_current0_r10bpa;	/**< 10 bit per Amp unit. */
	int32_t out_current1_r10bpa;	/**< 10 bit per Amp unit. */
};

/** Raw params. */
struct huapsu_params {
	int32_t online_out_voltage_r10bpv;  /**< 10 bit per Volt unit. */
	int32_t offline_out_voltage_r10bpv; /**< 10 bit per Volt unit. */
	int32_t overvolt_protec_r10bpv;	    /**< 10 bit per Volt unit. */
	int32_t online_current_limit;	    /**< Special (scaled) unit. */
	int32_t offline_current_limit;	    /**< Special (scaled) unit. */
};

/** Most common parameters, used by huawei PSU.
 *  TODO more parameters */
struct huapsu_config {
	uint32_t min_output_voltage_r10bpv; /**< 10 bit per Volt unit. */
	uint32_t max_output_voltage_r10bpv; /**< 10 bit per Volt unit. */
	uint32_t max_output_current_r10bpa; /**< 10 bit per Amp unit. */
	uint32_t max_output_power_r10bpw;   /**< 10 bit per Watt unit. */
};

/** Main instance. */
struct huapsu {
	uint8_t state;

	struct huapsu_config cfg;
	struct huapsu_data   data;
	struct huapsu_params params;

	struct huapsu_frame rx_frame;
	struct huapsu_frame tx_frame;

	bool online; /**< PSU is reported to be online. */
	bool busy;   /**< Should not yield. */

	/* Timers */
	/** How long are we allowed to stay offline. */
	uint32_t offline_timeout_r1ms;
	uint32_t offline_timer_r1ms; /**< How long we've been offline. */

	/* Diagnostics */
	uint8_t err_code;
	uint8_t last_msg_id;
	uint8_t last_answ_code;
};

extern const struct huapsu_config g_huapsu_config_r33640g1;
extern const struct huapsu_config g_huapsu_config_r4875g1;

void huapsu_init(struct huapsu *self, enum huapsu_hw_variant variant);

bool huapsu_parse_data_frame(struct huapsu	       *self,
			     const struct huapsu_frame *frame);

bool huapsu_parse_param_frame(struct huapsu		*self,
			      const struct huapsu_frame *frame);

bool huapsu_parse_input_frame(struct huapsu		*self,
			      const struct huapsu_frame *frame);

bool huapsu_push_rx_frame(struct huapsu		    *self,
			  const struct huapsu_frame *frame);

void uapsu_update_deltatime(struct huapsu *self, uint32_t delta_time_ms);

uint8_t huapsu_step(struct huapsu *self);

bool huapsu_is_busy(struct huapsu *self);

#ifndef HUAPSU_NO_IMPL
const struct huapsu_config g_huapsu_config_r33640g1 = {
    300U * 1024U, 400U * 1024U, 37U * 1024U, 15000U * 1024U};

const struct huapsu_config g_huapsu_config_r4875g1 = {
    42U * 1024U, 58U * 1024U, 75U * 1024U, 15000U * 1024U};

void huapsu_init(struct huapsu *self, enum huapsu_hw_variant variant)
{
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
	self->tx_frame.dlc = -1; /* Indicates that there is no frame. */
}

bool huapsu_parse_data_frame(struct huapsu	       *self,
			     const struct huapsu_frame *frame)
{
	bool success = false;

	uint8_t data_id = frame->data[1U];

	/** Value starts at 5th byte and has 32bit length.
	 *  Big-endian order (MSB goes first). */
	const uint32_t raw = ((uint32_t)frame->data[4U] << 24U) |
			     (uint32_t)frame->data[5U] << 16U |
			     (uint32_t)frame->data[6U] << 8U |
			     (uint32_t)frame->data[7U] << 0U;

	/** All values are signed integers. */
	const int32_t value = (int32_t)raw;

	self->last_answ_code = frame->data[0U];
	self->last_msg_id    = frame->data[1U];

	if (self->last_answ_code == (uint8_t)HUAPSU_ANSW_CODE_OK) {
		success = true;
	} else {
		data_id = HUAPSU_DATA_ID_INVALID;
	}

	switch (data_id) {
	case HUAPSU_DATA_ID_IN_POWER:
		self->data.in_power_r10bpw = value;
		break;

	case HUAPSU_DATA_ID_IN_FREQ:
		self->data.in_freq_r10bphz = value;
		break;

	case HUAPSU_DATA_ID_IN_CURRENT:
		self->data.in_current_r10bpa = value;
		break;

	case HUAPSU_DATA_ID_OUT_POWER:
		self->data.out_power_r10bpw = value;
		break;

	case HUAPSU_DATA_ID_EFFICIENCY:
		self->data.efficiency_r10bp100pct = value;
		break;

	case HUAPSU_DATA_ID_OUT_VOLTAGE:
		self->data.out_voltage_r10bpv = value;
		break;

	case HUAPSU_DATA_ID_OUT_CURRENT_MAX:
		self->data.out_current_max = value;
		break;

	case HUAPSU_DATA_ID_IN_VOLTAGE:
		self->data.out_voltage_r10bpv = value;
		break;

	case HUAPSU_DATA_ID_OUT_TEMP:
		self->data.out_temp_r10bpc = value;
		break;

	case HUAPSU_DATA_ID_OUT_CURRENT0:
		self->data.out_current0_r10bpa = value;
		break;

	case HUAPSU_DATA_ID_OUT_CURRENT1:
		self->data.out_current1_r10bpa = value;
		break;

	case HUAPSU_DATA_ID_UNKNOWN:
	case HUAPSU_DATA_ID_INVALID:
	default:
		self->err_code = HUAPSU_ERR_INVALID_DATA_ID;
		break;
	}

	return success;
}

bool huapsu_parse_param_frame(struct huapsu		*self,
			      const struct huapsu_frame *frame)
{
	bool success = false;

	self->last_answ_code = frame->data[0U];
	self->last_msg_id    = frame->data[1U];

	if (self->last_answ_code == (uint8_t)HUAPSU_ANSW_CODE_OK) {
		success = true;
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
			self->err_code = HUAPSU_ERR_INVALID_DLC;
		}
		break;

	case HUAPSU_MSG_ID_PARAM_ACK:
		if (frame->dlc == (int8_t)HUAPSU_FRAME_MAX_LEN) {
			success = huapsu_parse_param_frame(self, frame);
		} else {
			self->err_code = HUAPSU_ERR_INVALID_DLC;
		}
		break;

	default:
		self->err_code = HUAPSU_ERR_INVALID_ID;
		break;
	}

	return success;
}

/** Push RX frame into driver.
 *  Returns true on success and false if frame is already queued. */
bool huapsu_push_rx_frame(struct huapsu		    *self,
			  const struct huapsu_frame *frame)
{
	bool success = false;

	if (self->rx_frame.dlc < 0) {
		self->rx_frame = *frame;
		success	       = true;
	} else {
		success = false;
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

	/** TODO implement */

	/* Advance to the next state. */
	self->state = HUAPSU_STATE_TIMERS;

	return event;
}

uint8_t huapsu_state_timers(struct huapsu *self)
{
	self->state = HUAPSU_STATE_YIELD;
	return HUAPSU_EVENT_REQ_TIME;
}

void uapsu_update_deltatime(struct huapsu *self, const uint32_t delta_time_ms)
{
	/* Increment offline timer, until it reached max threshold */
	if (self->offline_timer_r1ms < self->offline_timeout_r1ms) {
		self->offline_timer_r1ms += delta_time_ms;
	} else {
		self->online = false;
	}
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

#endif /* HUAPSU_IMPL */
#endif /* HUAPSU_H */
