/* WIP demo source -- used for random testing and not for any 
 * particular demonstration.
 */
#include <stdio.h>
#include <math.h>

#include "t_base.h"
#include "t_sequence.h"
#include "t_draw.h"

#include "demos.h"
#include "keyboard.h"

/* Minimum delta in seconds between successive framebuffer 
 * rasterization operations. When t_poll() is used in non-blocking
 * mode, the UI/update loop may spin too quickly and cause serious 
 * visual artifacts when drawing to the terminal too quickly.
 *
 * On my system, a milisecond is more than enough to avoid those. This
 * is terminal dependent, I imagine.
 */
#define RASTERIZATION_DELTA_REQUIRED 1e-3

#define TERM_WIDTH_MIN 24
#define TERM_HEIGHT_MIN 16

static int
main_app();

/* see demos.h for more main selections */
#define SELECTED_MAIN main_app

int
main()
{
	return SELECTED_MAIN();
}

struct appctx
{
	struct t_frame  frame_primary;
	struct keyboard keyboard;

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

	struct {
		int32_t     tick_scale;

		/* describes the viewport coordinates on a midi roll. */
		int32_t     visual_lane;
		int32_t     visual_atom;

		/* describes the location of the user's cursor on the midi roll. */
		int32_t     cursor_index;
		int32_t     cursor_atom;
	} ed;
};

static inline void
app_do_human_staccato(
	struct appctx *ac,
	enum note note
) {
	keyboard_tone_activate(
		&ac->keyboard,
		ac->tm_now,
		ac->tm_staccato_sustain,
		keyboard_index_compose(ac->user_octave, note)
	);
}

static inline void
app_rasterize(
	struct appctx *ac
) {
	/* clear before drawing in case of error to display */
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
	t_frame_resize(&ac->frame_primary, term_w, term_h);
	t_frame_clear(&ac->frame_primary);

	int32_t const 
		frame_w = ac->frame_primary.width,
		frame_h = ac->frame_primary.height;

	keyboard_tones_deactivate_expired(&ac->keyboard, ac->tm_now);
	keyboard_draw(
		&ac->frame_primary, 
		&ac->keyboard, 
		ac->ed.visual_lane,
		0, 
		frame_h - KBD_OCTAVE_HEIGHT - 1,
		frame_w
	);

	/* rasterize */
	t_frame_rasterize(&ac->frame_primary, 0, 0);

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

static int
main_app()
{
	enum t_status stat;
	struct appctx ac;

	/* initialize */
	stat = keyboard_support_setup();
	if (stat < 0) {
		fprintf(stderr, "Failed to initialize virtual keyboard support: %s\n",
			t_status_string(stat)
		);
		goto e_init_keyboard_support;
	}

	stat = keyboard_create(&ac.keyboard);
	if (stat < 0) {
		fprintf(stderr, "Failed to create virtual keyboard: %s\n",
			t_status_string(stat)
		);
		goto e_init_keyboard;
	}

	stat = t_frame_create(&ac.frame_primary, 0, 0);
	if (stat < 0) {
		fprintf(stderr, "Failed to create primary framebuffer: %s\n",
			t_status_string(stat)
		);
		goto e_init_frame;
	}

	/* cosmetic initialization */
	int32_t term_w, term_h;
	t_termsize(&term_w, &term_h);

	ac.keyboard.color.frame_fg = T_RGB(96, 96, 96);
	ac.keyboard.color.frame_bg = T_WASHED;
	ac.keyboard.color.idle_white = T_RGB(96, 96, 96);
	ac.keyboard.color.idle_black = T_WASHED;
	ac.keyboard.color.active_white = T_RGB(128, 128, 128);
	ac.keyboard.color.active_black = T_RGB(96, 96, 96);

	ac.ed.visual_lane = keyboard_lane_compose_with_note(4, NOTE_C) - term_w / 2;

	/* setup, ui variables, and ui loop */
	t_setup();

	ac.should_run = true;
	ac.tm_last = t_elapsed();
	ac.tm_last_graphics = t_elapsed();
	
	ac.user_octave = 4;
	ac.tm_staccato_sustain = 0.1;
	ac.tm_required_delta_graphics = RASTERIZATION_DELTA_REQUIRED;

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
			app_do_human_staccato(&ac, NOTE_C);
			break;
		case T_POLL_CODE(0, '2'):
			app_do_human_staccato(&ac, NOTE_Cs);
			break;
		case T_POLL_CODE(0, 'w'):
			app_do_human_staccato(&ac, NOTE_D);
			break;
		case T_POLL_CODE(0, '3'):
			app_do_human_staccato(&ac, NOTE_Ds);
			break;
		case T_POLL_CODE(0, 'e'):
			app_do_human_staccato(&ac, NOTE_E);
			break;
		case T_POLL_CODE(0, 'r'):
			app_do_human_staccato(&ac, NOTE_F);
			break;
		case T_POLL_CODE(0, '5'):
			app_do_human_staccato(&ac, NOTE_Fs);
			break;
		case T_POLL_CODE(0, 't'):
			app_do_human_staccato(&ac, NOTE_G);
			break;
		case T_POLL_CODE(0, '6'):
			app_do_human_staccato(&ac, NOTE_Gs);
			break;
		case T_POLL_CODE(0, 'y'):
			app_do_human_staccato(&ac, NOTE_A);
			break;
		case T_POLL_CODE(0, '7'):
			app_do_human_staccato(&ac, NOTE_As);
			break;
		case T_POLL_CODE(0, 'u'):
			app_do_human_staccato(&ac, NOTE_B);
			break;

		/* administrative keybinds */
		case T_POLL_CODE(0, 'Q'):
			ac.should_run = 0;
			break;
		}
	}

	/* done */
	t_reset();
	t_clear();
	t_cleanup();

e_init_frame:
	t_frame_destroy(&ac.frame_primary);
e_init_keyboard:
	keyboard_destroy(&ac.keyboard);
e_init_keyboard_support:
	keyboard_support_cleanup();
	return stat > 0 ? 0 : 1;
}
