#include "lvgl.h"

#define PSU_BOX_ARM_DURATION 1000u

/******************************************************************************
 * CLASS
 *****************************************************************************/
enum psu_box_ui_arm_state {
	PSU_BOX_UI_STATE_DISABLED,
	PSU_BOX_UI_STATE_PRE_ARMED,
	PSU_BOX_UI_STATE_ARMED,
	PSU_BOX_UI_STATE_ENABLED
};

struct psu_box_ui {
	float min_voltage_V;
	float max_voltage_V;

	float min_current_A;
	float max_current_A;

	float set_voltage_V;
	float set_current_A;

	float out_voltage_V;
	float out_current_A;

	enum psu_box_ui_arm_state arm_state;

	bool charging_enabled;

	lv_obj_t *lbl_info;

	lv_obj_t *btn_voltage_plus;
	lv_obj_t *btn_voltage_minus;
	lv_obj_t *lbl_voltage_set;

	lv_obj_t *lbl_stats_out;

	lv_obj_t *btn_current_plus;
	lv_obj_t *btn_current_minus;
	lv_obj_t *lbl_current_set;

	lv_obj_t *btn_enabled;

	lv_obj_t *progress_bar;

	uint8_t _press_accel;
};

/******************************************************************************
 * PRIVATE
 *****************************************************************************/
void psu_box_ui_charge_enabled_update(struct psu_box_ui *self);
void psu_box_ui_stats_update(struct psu_box_ui *self);
void psu_box_ui_info_update(struct psu_box_ui *self, const char *info);
void psu_box_ui_voltage_setter_update(struct psu_box_ui *self);
void psu_box_ui_current_setter_update(struct psu_box_ui *self);

/* Fonts */
bool fix_w_get_glyph_dsc(const lv_font_t *font,
				lv_font_glyph_dsc_t *dsc, uint32_t letter,
                                uint32_t letter_next)
{
	bool ret = lv_font_get_glyph_dsc_fmt_txt(font, dsc, letter,
						 letter_next);
	if (!ret) return false;

	/* Set spacing between fonts same as line height*/
	dsc->adv_w = font->line_height / 1.5;

	dsc->ofs_x = (dsc->adv_w - dsc->box_w) / 2;

	return true;
}

/* Events */
void psu_box_ui_press_accelerate(struct psu_box_ui *self, lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if (code == LV_EVENT_LONG_PRESSED_REPEAT) {
		if (self->_press_accel < 5) {
			self->_press_accel++;
		}
	}

	if (code == LV_EVENT_RELEASED) {
		self->_press_accel = 1U;
	}
}

void psu_box_ui_evt_voltage_plus(lv_event_t * e)
{
	struct psu_box_ui *self = (struct psu_box_ui *)
						     lv_event_get_user_data(e);

	lv_event_code_t code = lv_event_get_code(e);

	psu_box_ui_press_accelerate(self, e);

	if ((code == LV_EVENT_LONG_PRESSED_REPEAT) ||
	    (code == LV_EVENT_CLICKED)) {
		self->set_voltage_V += self->_press_accel;

		psu_box_ui_voltage_setter_update(self);
	}
}

void psu_box_ui_evt_voltage_minus(lv_event_t * e)
{
	struct psu_box_ui *self = (struct psu_box_ui *)
						     lv_event_get_user_data(e);

	lv_event_code_t code = lv_event_get_code(e);

	psu_box_ui_press_accelerate(self, e);

	if ((code == LV_EVENT_LONG_PRESSED_REPEAT) ||
	    (code == LV_EVENT_CLICKED)) {
		self->set_voltage_V -= self->_press_accel;

		psu_box_ui_voltage_setter_update(self);
	}
}

void psu_box_ui_evt_current_plus(lv_event_t * e)
{
	struct psu_box_ui *self = (struct psu_box_ui *)
						     lv_event_get_user_data(e);

	lv_event_code_t code = lv_event_get_code(e);

	psu_box_ui_press_accelerate(self, e);

	if ((code == LV_EVENT_LONG_PRESSED_REPEAT) ||
	    (code == LV_EVENT_CLICKED)) {
		self->set_current_A += self->_press_accel;

		psu_box_ui_current_setter_update(self);
	}
}

void psu_box_ui_evt_current_minus(lv_event_t * e)
{
	struct psu_box_ui *self = (struct psu_box_ui *)
						     lv_event_get_user_data(e);

	lv_event_code_t code = lv_event_get_code(e);

	psu_box_ui_press_accelerate(self, e);

	if ((code == LV_EVENT_LONG_PRESSED_REPEAT) ||
	    (code == LV_EVENT_CLICKED)) {
		self->set_current_A -= self->_press_accel;

		psu_box_ui_current_setter_update(self);
	}
}

/* Progress BAR start (used to indicate ARM success) */
void psu_box_ui_progress_bar_start(struct psu_box_ui *self)
{
	static lv_style_t style_bg;
	static lv_style_t style_indic;

	lv_style_init(&style_bg);
	lv_style_set_border_color(&style_bg, lv_palette_main(LV_PALETTE_GREEN));
	lv_style_set_border_width(&style_bg, 2);
	lv_style_set_pad_all(&style_bg, 6); /*To make the indicator smaller*/
	lv_style_set_radius(&style_bg, 6);
	lv_style_set_anim_duration(&style_bg, PSU_BOX_ARM_DURATION);

	lv_style_init(&style_indic);
	lv_style_set_bg_opa(&style_indic, LV_OPA_COVER);
	lv_style_set_bg_color(&style_indic, lv_palette_main(LV_PALETTE_RED));
	lv_style_set_radius(&style_indic, 3);

	if (self->progress_bar == NULL) {
		self->progress_bar = lv_bar_create(lv_screen_active());
	}

	lv_obj_remove_style_all(self->progress_bar); /*To have a clean start*/
	lv_obj_add_style(self->progress_bar, &style_bg, 0);
	lv_obj_add_style(self->progress_bar, &style_indic, LV_PART_INDICATOR);

	lv_obj_set_size(self->progress_bar, 200, 10);
	lv_obj_set_pos(self->progress_bar, 60, 64);
	lv_bar_set_value(self->progress_bar, 100, LV_ANIM_ON);
}

/* Progress BAR stop (removes bar at all)*/
void psu_box_ui_progress_bar_stop(struct psu_box_ui *self)
{
	if (self->progress_bar != NULL) {
		lv_obj_delete(self->progress_bar);
		self->progress_bar = NULL;
	}
}

void psu_box_ui_evt_enabled(lv_event_t * e)
{
	struct psu_box_ui *self = (struct psu_box_ui *)
						     lv_event_get_user_data(e);

	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t *btn        = lv_event_get_target_obj(e);
	lv_obj_t *label      = lv_obj_get_child(btn, 0);

	/* This event always happens after button press */
	if (code == LV_EVENT_PRESSED) {
		if (self->arm_state == PSU_BOX_UI_STATE_DISABLED) {
			self->arm_state = PSU_BOX_UI_STATE_PRE_ARMED;
			lv_obj_set_style_bg_color(btn, lv_color_hex(0xFF7700), 0);
			lv_label_set_text(label, "ARM");
			psu_box_ui_progress_bar_start(self);
		} else {
			self->arm_state = PSU_BOX_UI_STATE_DISABLED;
			self->charging_enabled = false;
		}
	}

	/* This event always happens after long press */
	if (code == LV_EVENT_LONG_PRESSED) {
		if (self->arm_state == PSU_BOX_UI_STATE_PRE_ARMED) {
			self->arm_state = PSU_BOX_UI_STATE_ARMED;
		}
	}

	/* This event always happens after release */
	if (code == LV_EVENT_RELEASED) {
		if (self->arm_state == PSU_BOX_UI_STATE_ARMED) {
			self->arm_state = PSU_BOX_UI_STATE_ENABLED;
			self->charging_enabled = true;
		} else {
			self->arm_state = PSU_BOX_UI_STATE_DISABLED;
		}

		psu_box_ui_charge_enabled_update(self);

		if (self->charging_enabled == false) {
			psu_box_ui_progress_bar_stop(self);
		}
	}
}

/* Main */
void psu_box_ui_create(struct psu_box_ui *self)
{
	lv_obj_t *btn;
	lv_obj_t *label;

	static lv_font_t mono_font;
	static lv_font_t mono_font_small;

	mono_font = lv_font_montserrat_28;
	mono_font.get_glyph_dsc = fix_w_get_glyph_dsc;

	mono_font_small = lv_font_montserrat_20;
	mono_font_small.get_glyph_dsc = fix_w_get_glyph_dsc;

	/* Create root object */
	lv_obj_t *root = lv_screen_active();	

	/* Change root background color */
	lv_obj_set_style_bg_color(root, lv_color_hex(0x003a57),
				  LV_PART_MAIN);



	/* Device info label */
	label = lv_label_create(root);
	lv_obj_set_pos(label, 320/4, 10);
	lv_obj_set_size(label, 180, 50);
	lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF),
				    LV_PART_MAIN);
	lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, 0);
	self->lbl_info = label;

	psu_box_ui_info_update(self, "Device info is not available");



	/* Voltage plus */
	btn = lv_button_create(root);
	lv_obj_set_pos(btn, 10, 10);
	lv_obj_set_size(btn, 50, 50);
	lv_obj_add_event_cb(btn, psu_box_ui_evt_voltage_plus,
			    LV_EVENT_ALL, self);

	label = lv_label_create(btn);
	lv_obj_set_style_text_font(label, &mono_font, 0);
	lv_label_set_text(label, "V+");
	lv_obj_center(label);
 
	self->btn_voltage_plus = btn;

	/* Voltage label */
	label = lv_label_create(root);
	lv_obj_set_pos(label, 10, 100);
	lv_obj_set_size(label, 60, 100);
	lv_obj_set_style_text_color(label, lv_color_hex(0xffffff),
				    LV_PART_MAIN);
	lv_obj_set_style_text_font(label, &mono_font_small, 0);

	self->lbl_voltage_set = label;
	psu_box_ui_voltage_setter_update(self);

	/* Voltage minus */
	btn = lv_button_create(root);
	lv_obj_set_pos(btn, 10, 240 - 50 - 10);
	lv_obj_set_size(btn, 50, 50);
	lv_obj_add_event_cb(btn, psu_box_ui_evt_voltage_minus,
			    LV_EVENT_ALL, self);

	label = lv_label_create(btn);
	lv_obj_set_style_text_font(label, &mono_font, 0);
	lv_label_set_text(label, "V-");
	lv_obj_center(label);

	self->btn_voltage_minus = btn;



	/* Current plus */
	btn = lv_button_create(root);
	lv_obj_set_pos(btn, 320 - 50 - 10, 10);
	lv_obj_set_size(btn, 50, 50);
	lv_obj_add_event_cb(btn, psu_box_ui_evt_current_plus,
			    LV_EVENT_ALL, self);

	label = lv_label_create(btn);
	lv_obj_set_style_text_font(label, &mono_font, 0);
	lv_label_set_text(label, "A+");
	lv_obj_center(label);

	self->btn_current_plus = btn;

	/* Current label */
	label = lv_label_create(root);
	lv_obj_set_pos(label, 320 - 60, 100);
	lv_obj_set_size(label, 60, 100);
	lv_obj_set_style_text_color(label, lv_color_hex(0xffffff),
				    LV_PART_MAIN);
	lv_obj_set_style_text_font(label, &mono_font_small, 0);

	self->lbl_current_set = label;
	psu_box_ui_current_setter_update(self);

	/* Current minus */
	btn = lv_button_create(root);
	lv_obj_set_pos(btn, 320 - 50 - 10, 240 - 50 - 10);
	lv_obj_set_size(btn, 50, 50);
	lv_obj_add_event_cb(btn, psu_box_ui_evt_current_minus,
			    LV_EVENT_ALL, self);

	label = lv_label_create(btn);
	lv_obj_set_style_text_font(label, &mono_font, 0);
	lv_label_set_text(label, "A-");
	lv_obj_center(label);

	self->btn_current_minus = btn;

	/* Charge enable/disable */
	btn = lv_button_create(root);
	lv_obj_set_pos(btn, 320/4, 240 - 50 - 10);
	lv_obj_set_size(btn, 160, 50);
	lv_obj_set_style_bg_color(btn, lv_color_hex(0xff0000), 0);
	lv_obj_add_event_cb(btn, psu_box_ui_evt_enabled, LV_EVENT_ALL, self);

	label = lv_label_create(btn);
	lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
	lv_obj_center(label);

	self->btn_enabled = btn;
	psu_box_ui_charge_enabled_update(self);


	/* Measured stats */
	label = lv_label_create(root);
	lv_obj_set_pos(label, 320/4, 80);
	lv_obj_set_size(label, 130, 150);
	lv_obj_set_style_text_color(label, lv_color_hex(0x44FF44),
				    LV_PART_MAIN);
	lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_RIGHT, 0);
	lv_obj_set_style_text_font(label, &mono_font, 0);

	self->lbl_stats_out = label;
	psu_box_ui_stats_update(self);
}

/******************************************************************************
 * PUBLIC
 *****************************************************************************/
void psu_box_ui_init(struct psu_box_ui *self)
{
	self->min_voltage_V = 0.0f;
	self->max_voltage_V = 0.0f;

	self->max_current_A = 0.0f;
	self->max_current_A = 0.0f;

	self->set_voltage_V = 0.0f;
	self->set_current_A = 0.0f;

	self->out_voltage_V = 0.0f;
	self->out_current_A = 0.0f;

	self->arm_state = PSU_BOX_UI_STATE_DISABLED;

	self->charging_enabled = false;

	self->_press_accel = 1U;

	psu_box_ui_create(self);

	self->progress_bar = NULL;
}

/* UI update */
void psu_box_ui_info_update(struct psu_box_ui *self, const char *info)
{
	lv_label_set_text(self->lbl_info, info);
}

void psu_box_ui_stats_update(struct psu_box_ui *self)
{
	lv_label_set_text_fmt(self->lbl_stats_out, "% 3d V\n% 3d A\n% 3d W\n",
			      (int)self->out_voltage_V,
			      (int)self->out_current_A,
			     (int)(self->out_voltage_V * self->out_current_A));
}

void psu_box_ui_voltage_setter_update(struct psu_box_ui *self)
{
	if (self->set_voltage_V > self->max_voltage_V) {
		self->set_voltage_V = self->max_voltage_V;
	}

	if (self->set_voltage_V < self->min_voltage_V) {
		self->set_voltage_V = self->min_voltage_V;
	}

	lv_label_set_text_fmt(self->lbl_voltage_set, "SET% 3d",
			      (int)self->set_voltage_V);
}

void psu_box_ui_current_setter_update(struct psu_box_ui *self)
{
	if (self->set_current_A > self->max_current_A) {
		self->set_current_A = self->max_current_A;
	}

	if (self->set_current_A < self->min_current_A) {
		self->set_current_A = self->min_current_A;
	}

	lv_label_set_text_fmt(self->lbl_current_set, "SET% 3d",
			      (int)self->set_current_A);
}

void psu_box_ui_charge_enabled_update(struct psu_box_ui *self)
{
	lv_obj_t *label = lv_obj_get_child(self->btn_enabled, 0);

	if (self->charging_enabled) {
		lv_obj_set_style_bg_color(self->btn_enabled,
					  lv_color_hex(0x770000), 0);
		lv_label_set_text(label, "STOP");
	} else {
		lv_obj_set_style_bg_color(self->btn_enabled,
					  lv_color_hex(0x007700), 0);
		lv_label_set_text(label, "START");
	}
}

void psu_box_ui_update(struct psu_box_ui *self)
{
	psu_box_ui_stats_update(self);
	psu_box_ui_voltage_setter_update(self);
	psu_box_ui_current_setter_update(self);
	psu_box_ui_charge_enabled_update(self);
}
