/*
 * vgencoder.h
 *
 * Created: 03.03.2022 13:58:57
 *  Author: PavlovVG
 */

#ifndef ENCODER_LIB_H_
#define ENCODER_LIB_H_

#include <stdint.h>
#include <stdbool.h>

#define VGENCODER_TURNED_LEFT	0x1
#define VGENCODER_TURNED_RIGHT	0x2

typedef enum
{
	VGENCODER_WAITING_FOR_INITIAL_STATE,
	VGENCODER_WAITING_FOR_ROTATION_STATE
} vgencoder_processing_state_t;

typedef enum
{
	VGENCODER_WAITING_FOR_BUTTON_PRESS_STATE,
	VGENCODER_WAITING_FOR_BUTTON_DEBOUNCE_STATE,
	VGENCODER_WAITING_FOR_BUTTON_HOLD_STATE,
	VGENCODER_BUTTON_HOLD_STATE,
	VGENCODER_HOLD_TURN
} vgencoder_button_processing_state_t;

struct vgencoder
{
	uint8_t (*get_encoder_state)(void *);
	uint8_t (*get_encoder_button_state)(void *);
	void *get_encoder_state_context;
	uint8_t period;
	uint16_t button_hold_timeout;
	vgencoder_processing_state_t encoder_processing_state;
	vgencoder_button_processing_state_t encoder_button_processing_state;
	uint8_t state;
	uint8_t button_debounce_ticks;
	uint8_t button_debounce_counter;
	uint16_t button_hold_ticks;
	uint16_t button_hold_counter;
};

int8_t vgencoder_init(struct vgencoder *self,
					uint8_t (*get_encoder_state)(void *),
					uint8_t (*get_encoder_button_state)(void *),
					void *get_encoder_state_context,
					uint8_t period_ms,
					uint16_t button_hold_timeout_ms);

void vgencoder_processing(struct vgencoder *self);
void vgencoder_button_processing(struct vgencoder *self);

bool vgencoder_is_right(struct vgencoder *self);
bool vgencoder_is_left(struct vgencoder *self);
bool vgencoder_is_hold_right(struct vgencoder *self);
bool vgencoder_is_hold_left(struct vgencoder *self);
bool vgencoder_button_is_click(struct vgencoder *self);
bool vgencoder_button_is_hold(struct vgencoder *self);

#endif /* ENCODER_LIB_H_ */
