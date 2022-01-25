#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <unistd.h>

#include "t_base.h"
#include "t_sequence.h"
#include "t_draw.h"

#include "keyboard_model.h"

/* Minimum delta in seconds between successive framebuffer 
 * rasterization operations. When t_poll() is used in non-blocking
 * mode, the UI/update loop may spin too quickly and cause serious 
 * visual artifacts when drawing to the terminal too quickly.
 *
 * On my system, a milisecond is more than enough to avoid those. This
 * is terminal dependent, I imagine.
 */
#define RASTERIZATION_DELTA_REQUIRED 1e-3

int
main(void)
{
	enum t_status stat;

	/* initialize supports */
	stat = keyboard_model_support_init();
	if (stat < 0) {
		fprintf(stderr, "Failed to initialize virtual keyboard support: %s\n",
			t_status_string(stat)
		);
		goto e_initialization;
	}

	struct t_frame frame_primary;
	stat = t_frame_create(&frame_primary, 0, 0);
	if (stat < 0) {
		fprintf(stderr, "Failed to create primary framebuffer: %s\n",
			t_status_string(stat)
		);
		goto e_initialization;
	}

	struct keyboard keyboard = {
		.midi_index_start = MIDI_INDEX(NOTE_C, 4),
		.midi_index_end   = MIDI_INDEX(NOTE_B, 4),
	};

	/* setup, ui variables, and ui loop */
	t_setup();

	double tm_graphics_last = t_elapsed();
	bool should_poll_wait = true;
	bool should_run = true;

	double tm_staccato_sustain = 1;
	int32_t user_octave = 4;

#define MAIN__HUMAN_STACCATO(note)     \
	keyboard_human_staccato(           \
		&keyboard,                     \
		tm_staccato_sustain,           \
		MIDI_INDEX(note, user_octave)  \
	)

	while (should_run) {
		double tm_now = t_elapsed();
		double tm_graphics_delta = tm_now - tm_graphics_last;

		if (tm_graphics_delta >= RASTERIZATION_DELTA_REQUIRED) {
			tm_graphics_last = tm_now;

			/* update framebuffer */
			int32_t term_w, term_h;
			t_termsize(&term_w, &term_h);

			t_frame_resize(&frame_primary, term_w, term_h);
			t_frame_clear(&frame_primary);

			t_draw

			keyboard_draw(&frame_primary);

			/* rasterize */
			t_clear();
			t_frame_rasterize(&frame_octave, 0, 0);
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

e_initialization:
	t_frame_destroy(&frame_primary);
	keyboard_support_cleanup();
	return 0;
}
