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

static int
main_application();

#define SELECTED_MAIN main_application

int
main()
{
	return SELECTED_MAIN();
}

static int
main_application()
{
	enum t_status stat;

	/* initialize supports */
	stat = keyboard_support_setup();
	if (stat < 0) {
		fprintf(stderr, "Failed to initialize virtual keyboard support: %s\n",
			t_status_string(stat)
		);
		goto e_init_keyboard;
	}

	struct t_frame frame_primary;
	stat = t_frame_create(&frame_primary, 0, 0);
	if (stat < 0) {
		fprintf(stderr, "Failed to create primary framebuffer: %s\n",
			t_status_string(stat)
		);
		goto e_init_frame;
	}

	struct keyboard keyboard = {
		.mi_lo = MIDI_INDEX(NOTE_C, 3),
		.mi_hi = MIDI_INDEX(NOTE_B, 5),
	};

	/* setup, ui variables, and ui loop */
	t_setup();

	double tm_graphics_last = t_elapsed();
	bool should_run = true;

	double tm_staccato_sustain = 0.25;
	uint8_t gry = 0;
	int32_t user_octave = 4;

#define MAIN__HUMAN_STACCATO(note)     \
	keyboard_tone_activate(            \
		&keyboard,                     \
		tm_now,                        \
		tm_staccato_sustain,           \
		MIDI_INDEX(note, user_octave)  \
	)

	while (should_run) {
		double tm_now = t_elapsed();
		double tm_graphics_delta = tm_now - tm_graphics_last;

		if (tm_graphics_delta >= 0) {
			tm_graphics_last = tm_now;

			/* update framebuffer */
			int32_t term_w, term_h;
			t_termsize(&term_w, &term_h);

			t_frame_resize(&frame_primary, term_w, term_h);
			t_frame_clear(&frame_primary);

			gry = (uint8_t) (127 * sin(tm_now) + 128);
			keyboard_tones_deactivate_expired(&keyboard, tm_now);
			keyboard_draw(&frame_primary, &keyboard,
				T_RGB(gry, gry, gry), T_WASHED,
				T_RGB(192, 128,  64), T_WASHED,
				0, 0
			);

			/* rasterize */
			t_reset();
			t_clear();

			t_frame_rasterize(&frame_primary, 0, 0);

#ifdef TC_DEBUG_METRICS
			t_cursor_pos(1, term_h);
			t_foreground_256_ex(0, 0, 0);
			t_background_256_ex(255, 255, 255);
			t_write_f("Stored: %u, Flushed: %u, GRY: %u",
				t_debug_write_nstored(),
				t_debug_write_nflushed(),
				gry
			);
			t_debug_write_metrics_clear();
#endif
			t_flush();
		}

		switch (t_poll()) {
		/* keyboard keybinds */
		case T_POLL_CODE(0, 'q'):
			MAIN__HUMAN_STACCATO(NOTE_C);
			break;
		case T_POLL_CODE(0, '2'):
			MAIN__HUMAN_STACCATO(NOTE_Cs);
			break;
		case T_POLL_CODE(0, 'w'):
			MAIN__HUMAN_STACCATO(NOTE_D);
			break;
		case T_POLL_CODE(0, '3'):
			MAIN__HUMAN_STACCATO(NOTE_Ds);
			break;
		case T_POLL_CODE(0, 'e'):
			MAIN__HUMAN_STACCATO(NOTE_E);
			break;
		case T_POLL_CODE(0, 'r'):
			MAIN__HUMAN_STACCATO(NOTE_F);
			break;
		case T_POLL_CODE(0, '5'):
			MAIN__HUMAN_STACCATO(NOTE_Fs);
			break;
		case T_POLL_CODE(0, 't'):
			MAIN__HUMAN_STACCATO(NOTE_G);
			break;
		case T_POLL_CODE(0, '6'):
			MAIN__HUMAN_STACCATO(NOTE_Gs);
			break;
		case T_POLL_CODE(0, 'y'):
			MAIN__HUMAN_STACCATO(NOTE_A);
			break;
		case T_POLL_CODE(0, '7'):
			MAIN__HUMAN_STACCATO(NOTE_As);
			break;
		case T_POLL_CODE(0, 'u'):
			MAIN__HUMAN_STACCATO(NOTE_B);
			break;

		/* administrative keybinds */
		case T_POLL_CODE(0, 'Q'):
			should_run = 0;
			break;
		}
	}

	/* done */
	t_frame_destroy(&frame_primary);
	t_reset();
	t_clear();
	t_cleanup();

e_init_frame:
	t_frame_destroy(&frame_primary);
e_init_keyboard:
	keyboard_support_cleanup();
	return stat > 0 ? 0 : 1;
}
