/* WIP demo source -- used for random testing and not for any 
 * particular demonstration.
 */
#include <stdio.h>
#include <math.h>

#include "t_base.h"
#include "t_sequence.h"
#include "t_draw.h"

#include "roll.h"

/* Minimum delta in seconds between successive framebuffer 
 * rasterization operations. When t_poll() is used in non-blocking
 * mode, the UI/update loop may spin too quickly and cause serious 
 * visual artifacts when drawing to the terminal too quickly.
 *
 * On my system, a milisecond is more than enough to avoid those. This
 * is terminal dependent, I imagine.
 */
#define RASTERIZATION_DELTA_REQUIRED 8e-3

#define TERM_WIDTH_MIN 24
#define TERM_HEIGHT_MIN 16

struct appctx
{
	struct t_frame  frame;
	struct roll     roll;

	/* */
	bool            should_run;
	double          tm_now;
	double          tm_last;
	double          tm_last_graphics;
	double          tm_delta_graphics;

	/* user configurable */
	int32_t         user_octave;
	double          tm_staccato_sustain;
	double          tm_required_delta_graphics;
};

static inline enum t_status
app_initialize(
	struct appctx *ac
) {
	enum t_status stat;
	stat = t_frame_create(&ac->frame, 0, 0);
	if (stat < 0) {
		fprintf(stderr, "Failed to create primary framebuffer: %s\n",
			t_status_string(stat)
		);
		return stat;
	}

	roll_init(&ac->roll);

	/* finalize */
	t_setup();

	ac->should_run = true;
	ac->tm_last = t_elapsed();
	ac->tm_last_graphics = t_elapsed();
	
	ac->user_octave = 4;
	ac->tm_staccato_sustain = 0.1;
	ac->tm_required_delta_graphics = RASTERIZATION_DELTA_REQUIRED;

	/* cosmetic initialization */
	int32_t term_w, term_h;
	t_termsize(&term_w, &term_h);

#if 0
	ac->keyboard.color.frame_fg = T_RGB(96, 96, 96);
	ac->keyboard.color.frame_bg = T_WASHED;
	ac->keyboard.color.idle_white = T_RGB(96, 96, 96);
	ac->keyboard.color.idle_black = T_WASHED;
	ac->keyboard.color.active_white = T_RGB(128, 128, 128);
	ac->keyboard.color.active_black = T_RGB(96, 96, 96);

	ac->ed.visual_lane = keyboard_lane_compose_with_note(4, NOTE_C) - term_w / 2;
#endif

	return T_OK;
}

static inline int
app_destroy(
	struct appctx *ac
) {
	t_frame_destroy(&ac->frame);
	t_cleanup();
	return 0;
}

static inline void
app_rasterize(
	struct appctx *ac
) {
	/* @TODO(max): double buffer */
	t_reset();
	t_clear();

	/* */
	int32_t term_w, term_h;
	t_termsize(&term_w, &term_h);

	if (term_w < TERM_WIDTH_MIN || term_h < TERM_HEIGHT_MIN) {
		t_foreground_256_ex(255, 255, 255);
		t_background_256_ex(255, 0, 0);
		t_cursor_pos(1, 1);
		t_write_f("ERR TERM SIZE %dx%d < %dx%d",
			term_w, term_h,
			TERM_WIDTH_MIN, 
			TERM_HEIGHT_MIN
		);
		t_flush();
		return;
	}

	/* */
	t_frame_resize(&ac->frame, term_w, term_h);
	t_frame_clear(&ac->frame);

	int32_t const 
		frame_w = ac->frame.width,
		frame_h = ac->frame.height;

	/* */
	roll_draw(&ac->frame, &ac->roll, frame_w - 2, frame_h - 2, 1, 1);

#if 0
	keyboard_tones_deactivate_expired(&ac->keyboard, ac->tm_now);
	keyboard_draw(
		&ac->frame, 
		&ac->keyboard, 
		ac->ed.visual_lane,
		0, 
		frame_h - KBD_OCTAVE_HEIGHT - 1,
		frame_w
	);
#endif

	/* rasterize */
	t_frame_rasterize(&ac->frame, 0, 0);

#ifdef TC_DEBUG_METRICS
	t_cursor_pos(1, term_h);
	t_foreground_256_ex(0, 0, 0);
	t_background_256_ex(255, 255, 255);
	t_write_f("Stored: %u, Flushed: %u, W: %d, H: %d",
		t_debug_write_nstored(),
		t_debug_write_nflushed(),
		term_w,
		term_h
	);
	t_debug_write_metrics_clear();
#endif
	t_flush();
}

int
main()
{
	enum t_status stat;
	struct appctx ac;

	stat = app_initialize(&ac);
	if (stat < 0) {
		goto e_init;
	}

	while (ac.should_run) {
		ac.tm_now = t_elapsed();
		ac.tm_delta_graphics = ac.tm_now - ac.tm_last_graphics;

		if (ac.tm_delta_graphics >= ac.tm_required_delta_graphics) {
			app_rasterize(&ac);
			ac.tm_last_graphics = ac.tm_now;
		}

		switch (t_poll()) {
		/* keyboard keybinds */
		case T_POLL_CODE(0, 'q'):
			roll_tone_toggle(&ac.roll, NOTE_C);
			break;
		case T_POLL_CODE(0, '2'):
			roll_tone_toggle(&ac.roll, NOTE_Cs);
			break;
		case T_POLL_CODE(0, 'w'):
			roll_tone_toggle(&ac.roll, NOTE_D);
			break;
		case T_POLL_CODE(0, '3'):
			roll_tone_toggle(&ac.roll, NOTE_Ds);
			break;
		case T_POLL_CODE(0, 'e'):
			roll_tone_toggle(&ac.roll, NOTE_E);
			break;
		case T_POLL_CODE(0, 'r'):
			roll_tone_toggle(&ac.roll, NOTE_F);
			break;
		case T_POLL_CODE(0, '5'):
			roll_tone_toggle(&ac.roll, NOTE_Fs);
			break;
		case T_POLL_CODE(0, 't'):
			roll_tone_toggle(&ac.roll, NOTE_G);
			break;
		case T_POLL_CODE(0, '6'):
			roll_tone_toggle(&ac.roll, NOTE_Gs);
			break;
		case T_POLL_CODE(0, 'y'):
			roll_tone_toggle(&ac.roll, NOTE_A);
			break;
		case T_POLL_CODE(0, '7'):
			roll_tone_toggle(&ac.roll, NOTE_As);
			break;
		case T_POLL_CODE(0, 'u'):
			roll_tone_toggle(&ac.roll, NOTE_B);
			break;

		/* administrative keybinds */
		case T_POLL_CODE(0, 'Q'):
			ac.should_run = 0;
			break;
		}
	}

	t_reset();
	t_clear();
	t_cleanup();

e_init:
	app_destroy(&ac);
	return stat < 0 ? 1 : 0;
}
