/*
 * vgencoder.c
 *
 * Created: 03.03.2022 13:58:48
 *  Author: PavlovVG
 */

#include "vgencoder.h"
#include <stdlib.h>

#define ENCODER_INITIAL_STATE 		0x3
#define ENCODER_ERROR_STATE 		0x0
#define ENCODER_RIGHT_STATE 		0x1
#define ENCODER_LEFT_STATE 			0x2
#define ENCODER_HOLD_RIGHT_STATE	0x4
#define ENCODER_HOLD_LEFT_STATE		0x8
#define ENCODER_BUTTON_CLICK_STATE	0x10
#define ENCODER_BUTTON_HOLD_STATE	0x20

#define ENCODER_HOLD_OFFSET	0x2

#define BUTTON_PRESSED 	1
#define BUTTON_RELEASED	0

#define BUTTON_DEBOUNCE_TIME_MS 50

#define INIT_ERROR	(int8_t)-1
#define INIT_OK		(int8_t)0

int8_t vgencoder_init(struct vgencoder *self,
					  uint8_t (*get_encoder_state)(void *),
					  uint8_t (*get_encoder_button_state)(void *),
					  void *get_encoder_state_context,
					  uint8_t period_ms,
					  uint16_t button_hold_timeout_ms)
{
	if (self == NULL || get_encoder_state == NULL || get_encoder_button_state == NULL)
	{
		return INIT_ERROR;
	}
	self->get_encoder_state = get_encoder_state;
	self->get_encoder_button_state = get_encoder_button_state;
	self->get_encoder_state_context = get_encoder_state_context;
	self->period = period_ms;
	self->button_hold_timeout = button_hold_timeout_ms;
	self->encoder_processing_state = VGENCODER_WAITING_FOR_INITIAL_STATE;
	self->encoder_button_processing_state = VGENCODER_WAITING_FOR_BUTTON_PRESS_STATE;
	self->state = 0;
	self->button_debounce_ticks = BUTTON_DEBOUNCE_TIME_MS / period_ms;
	self->button_debounce_counter = self->button_debounce_ticks;
	self->button_hold_ticks = button_hold_timeout_ms / period_ms;
	self->button_hold_counter = self->button_hold_ticks;
	return INIT_OK;
}

void vgencoder_processing(struct vgencoder *self)
{
	uint8_t new_state = self->get_encoder_state(self->get_encoder_state_context);
	if (self->encoder_processing_state == VGENCODER_WAITING_FOR_INITIAL_STATE &&
		new_state == ENCODER_INITIAL_STATE)
	{
		self->encoder_processing_state = VGENCODER_WAITING_FOR_ROTATION_STATE;
		return;
	}
	if (self->encoder_processing_state == VGENCODER_WAITING_FOR_INITIAL_STATE ||
		new_state == ENCODER_INITIAL_STATE)
	{
		return;
	}
	if (new_state == ENCODER_ERROR_STATE)
	{
		self->encoder_processing_state = VGENCODER_WAITING_FOR_INITIAL_STATE;
		return;
	}
	if (self->get_encoder_button_state(self->get_encoder_state_context) == BUTTON_PRESSED)
	{
		self->state |= (new_state << ENCODER_HOLD_OFFSET);
		self->encoder_button_processing_state = VGENCODER_HOLD_TURN;
		self->encoder_processing_state = VGENCODER_WAITING_FOR_INITIAL_STATE;
		return;
	}
	self->state |= new_state;
	self->encoder_processing_state = VGENCODER_WAITING_FOR_INITIAL_STATE;
}

static void button_debounce(struct vgencoder *self, const uint8_t state)
{
	if (--self->button_debounce_counter != 0)
	{
		return;
	}
	if (state == BUTTON_RELEASED)
	{
		self->button_debounce_counter = self->button_debounce_ticks;
		self->encoder_button_processing_state = VGENCODER_WAITING_FOR_BUTTON_PRESS_STATE;
		return;
	}
	self->button_debounce_counter = self->button_debounce_ticks;
	self->encoder_button_processing_state = VGENCODER_WAITING_FOR_BUTTON_HOLD_STATE;
}

static void button_hold(struct vgencoder *self, const uint8_t state)
{
	if (state == BUTTON_RELEASED)
	{
		self->state |= ENCODER_BUTTON_CLICK_STATE;
		self->button_hold_counter = self->button_hold_ticks;
		self->encoder_button_processing_state = VGENCODER_WAITING_FOR_BUTTON_PRESS_STATE;
	}
	if (--self->button_hold_counter == 0)
	{
		self->state |= ENCODER_BUTTON_HOLD_STATE;
		self->button_hold_counter = self->button_hold_ticks;
		self->encoder_button_processing_state = VGENCODER_BUTTON_HOLD_STATE;
	}
}

void vgencoder_button_processing(struct vgencoder *self)
{
	uint8_t state = self->get_encoder_button_state(self->get_encoder_state_context);
	if (self->encoder_button_processing_state == VGENCODER_WAITING_FOR_BUTTON_PRESS_STATE &&
		state == BUTTON_PRESSED)
	{
		self->encoder_button_processing_state = VGENCODER_WAITING_FOR_BUTTON_DEBOUNCE_STATE;
	}
	if (self->encoder_button_processing_state == VGENCODER_WAITING_FOR_BUTTON_PRESS_STATE)
	{
		return;
	}
	if (self->encoder_button_processing_state == VGENCODER_WAITING_FOR_BUTTON_DEBOUNCE_STATE)
	{
		button_debounce(self, state);
	}
	if (self->encoder_button_processing_state == VGENCODER_WAITING_FOR_BUTTON_HOLD_STATE)
	{
		button_hold(self, state);
	}
	if (state == BUTTON_RELEASED &&
		self->encoder_button_processing_state == VGENCODER_HOLD_TURN)
	{
		self->encoder_button_processing_state = VGENCODER_WAITING_FOR_BUTTON_PRESS_STATE;
		return;
	}
	if (state == BUTTON_RELEASED &&
		self->encoder_button_processing_state == VGENCODER_BUTTON_HOLD_STATE)
	{
		self->encoder_button_processing_state = VGENCODER_WAITING_FOR_BUTTON_PRESS_STATE;
		return;
	}
}

bool vgencoder_is_right(struct vgencoder *self)
{
	if (self->state & ENCODER_RIGHT_STATE)
	{
		self->state &= ~ENCODER_RIGHT_STATE;
		return true;
	}
	return false;
}

bool vgencoder_is_left(struct vgencoder *self)
{
	if (self->state & ENCODER_LEFT_STATE)
	{
		self->state &= ~ENCODER_LEFT_STATE;
		return true;
	}
	return false;
}

bool vgencoder_is_hold_right(struct vgencoder *self)
{
	if (self->state & ENCODER_HOLD_RIGHT_STATE)
	{
		self->state &= ~ENCODER_HOLD_RIGHT_STATE;
		return true;
	}
	return false;
}

bool vgencoder_is_hold_left(struct vgencoder *self)
{
	if (self->state & ENCODER_HOLD_LEFT_STATE)
	{
		self->state &= ~ENCODER_HOLD_LEFT_STATE;
		return true;
	}
	return false;
}

bool vgencoder_button_is_click(struct vgencoder *self)
{
	if (self->state & ENCODER_BUTTON_CLICK_STATE)
	{
		self->state &= ~ENCODER_BUTTON_CLICK_STATE;
		return true;
	}
	return false;
}

bool vgencoder_button_is_hold(struct vgencoder *self)
{
	if (self->state & ENCODER_BUTTON_HOLD_STATE)
	{
		self->state &= ~ENCODER_BUTTON_HOLD_STATE;
		return true;
	}
	return false;
}
