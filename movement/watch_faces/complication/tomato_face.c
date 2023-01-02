/*
 * MIT License
 *
 * Copyright (c) 2022 Wesley Ellis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFtomato_ringEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdlib.h>
#include <string.h>
#include "tomato_face.h"
#include "watch_utility.h"

static int16_t FocusTime = 25 * 60;
static int16_t BreakTime = 5 * 60;

static void tomato_draw(tomato_state_t *state) {
  char kind = (state->kind == tomato_break) ? 'b' : 'f';
  uint16_t delta = state->target - state->elapsed;
  div_t result = div(delta, 60);
  uint8_t min = result.quot;
  uint8_t sec = result.rem;

  char buf[16];
  sprintf(buf, "TO %c%2d%02d%2d", kind, min, sec, state->done_count);
  watch_display_string(buf, 0);
}

static void tomato_pause(tomato_state_t *state) {
  state->mode = tomato_ready;
  watch_clear_indicator(WATCH_INDICATOR_BELL);
}

static void tomato_reset(tomato_state_t *state) {
  state->elapsed = 0;
  state->target = (state->kind == tomato_focus) ? FocusTime : BreakTime;
  tomato_pause(state);
}

static void tomato_ring(tomato_state_t *state) {
  movement_play_signal();
  if (state->kind == tomato_focus) {
	state->kind = tomato_break;
	state->done_count++;
  } else {
	state->kind = tomato_focus;
  }
  tomato_reset(state);
}

void tomato_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr) {
  (void) settings;
  (void) watch_face_index;

  if (*context_ptr == NULL) {
	*context_ptr = malloc(sizeof(tomato_state_t));
	tomato_state_t *state = (tomato_state_t*)*context_ptr;
	memset(*context_ptr, 0, sizeof(tomato_state_t));
	state->mode = tomato_ready;
	state->kind= tomato_focus;
	state->done_count = 0;
	tomato_reset(state);
  }
}

void tomato_face_activate(movement_settings_t *settings, void *context) {
  tomato_state_t *state = (tomato_state_t *)context;
  state->mode = tomato_ready;
  watch_set_colon();
}

bool tomato_face_loop(movement_event_t event, movement_settings_t *settings, void *context) {
  tomato_state_t *state = (tomato_state_t *)context;
  switch (event.event_type) {
  case EVENT_ACTIVATE:
	tomato_draw(state);
	break;
  case EVENT_TICK:
	if (state->mode == tomato_run) state->elapsed++;
	if (state->elapsed >= state->target) tomato_ring(state);
	tomato_draw(state);
	break;
  case EVENT_MODE_BUTTON_UP:
	movement_move_to_next_face();
	break;
  case EVENT_LIGHT_BUTTON_UP:
	// Toggle between break and focus mode
	if (state->mode == tomato_ready) {
	  state->kind = (state->kind == tomato_break) ? tomato_focus : tomato_break;
	  tomato_reset(state);
	}
	tomato_draw(state);
	break;
  case EVENT_ALARM_BUTTON_UP:
	// Toggle between run and pause
	if (state->mode == tomato_run) {
	  tomato_pause(state);
	} else {
	  state->mode = tomato_run;
	  watch_set_indicator(WATCH_INDICATOR_BELL);
	}
	tomato_draw(state);
	break;
  case EVENT_ALARM_LONG_PRESS:
	// Reset all data
	state->done_count = 0;
	state->kind = tomato_focus;
	tomato_reset(state);
	break;
  default:
	break;
  }

  return true;
}

void tomato_face_resign(movement_settings_t *settings, void *context) {
  (void) settings;
  (void) context;
}
