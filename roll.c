#include <stdio.h>
#include <float.h>

#include "t_base.h"
#include "t_sequence.h"
#include "t_draw.h"

#include "roll.h"


static char const * const g_kbd_frame =
	            "+-------------+\n"
	            "||b|b|||b|b|b||\n"
	            "||b|b|||b|b|b||\n"
	            "|+-+-+|+-+-+-+|\n"
	            "|w|w|w|w|w|w|w|\n"
	            "|w|w|w|w|w|w|w|\n"
	            "+-+-+-+-+-+-+-+\n";

static char const *const g_kbd_notes [KBD_NOTES] = {
	[NOTE_C ] = "               \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            " W             \n"
	            " W             \n"
	            "               \n",

	[NOTE_Cs] = "               \n"
	            "  B            \n"
	            "  B            \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            "               \n",

	[NOTE_D ] = "               \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            "   W           \n"
	            "   W           \n"
	            "               \n",

	[NOTE_Ds] = "               \n"
	            "    B          \n"
	            "    B          \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            "               \n",

	[NOTE_E ] = "               \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            "     W         \n"
	            "     W         \n"
	            "               \n",

	[NOTE_F ] = "               \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            "       W       \n"
	            "       W       \n"
	            "               \n",

	[NOTE_Fs] = "               \n"
	            "        B      \n"
	            "        B      \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            "               \n",

	[NOTE_G ] = "               \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            "         W     \n"
	            "         W     \n"
	            "               \n",

	[NOTE_Gs] = "               \n"
	            "          B    \n"
	            "          B    \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            "               \n",

	[NOTE_A ] = "               \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            "           W   \n"
	            "           W   \n"
	            "               \n",

	[NOTE_As] = "               \n"
	            "            B  \n"
	            "            B  \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            "               \n",

	[NOTE_B ] = "               \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            "             W \n"
	            "             W \n"
	            "               \n",
};

static inline void
keyboard__zero(
	struct keyboard *kbd
) {
	kbd->color.frame_fg = 0;
	kbd->color.frame_bg = 0;

	kbd->color.idle_white = 0;
	kbd->color.idle_black = 0;

	kbd->color.active_white = 0;
	kbd->color.active_black = 0;

	kbd->_tone_pointer = 0;
}

static inline void
keyboard__defaults(
	struct keyboard *kbd
) {
	kbd->color.frame_fg = T_WASHED;
	kbd->color.frame_bg = T_WASHED;

	kbd->color.idle_white = T_RGB(255, 255, 255);
	kbd->color.idle_black = T_WASHED;

	kbd->color.active_white = T_RGB(255, 192, 192);
	kbd->color.active_black = T_RGB(255, 192, 192);
}

void 
keyboard_init(
	struct keyboard *kbd
) {
	keyboard__zero(kbd);
	keyboard__defaults(kbd);
}

void
keyboard_tone_activate(
	struct keyboard *kbd,
	double tm_start,
	double tm_sustain,
	int32_t index
) {
	/* check if note is already activated */
	for (int32_t i = 0; i < kbd->_tone_pointer; ++i) {
		struct tone *tone = &kbd->_tones_active[i];
		if (tone->_index == index) {
			return;
		}
	}

	/* it tone is not active, check to make sure we have room */
	if (kbd->_tone_pointer >= KBD_POLYPHONY) return;

	/* otherwise activate it */
	struct tone *tone = 
		&kbd->_tones_active[kbd->_tone_pointer++];
	tone->_tm_start = tm_start;
	tone->_tm_sustain = tm_sustain;
	tone->_index = index;
}

void
keyboard_tone_deactivate(
	struct keyboard *kbd,
	int32_t index 
) {
	for (int32_t i = 0; i < kbd->_tone_pointer; ++i) {
		struct tone *tone = &kbd->_tones_active[i];
		if (tone->_index != index) {
			continue;
		}

		--kbd->_tone_pointer;
		if (i == kbd->_tone_pointer) {
			continue;
		}

		struct tone *end = 
			&kbd->_tones_active[kbd->_tone_pointer];
		tone->_tm_start   = end->_tm_start;
		tone->_tm_sustain = end->_tm_sustain;
		tone->_index      = end->_index;
		--i;
	}
}

void
keyboard_tones_deactivate_expired(
	struct keyboard *kbd,
	double tm_point
) {
	for (int32_t i = 0; i < kbd->_tone_pointer; ++i) {
		struct tone *tone = &kbd->_tones_active[i];
		if (tone->_tm_start + tone->_tm_sustain > tm_point) {
			continue;
		}

		--kbd->_tone_pointer;
		if (i == kbd->_tone_pointer) {
			continue;
		}

		struct tone *end = 
			&kbd->_tones_active[kbd->_tone_pointer];
		tone->_tm_start   = end->_tm_start;
		tone->_tm_sustain = end->_tm_sustain;
		tone->_index      = end->_index;
		--i;
	}
}

enum t_status
keyboard_draw(
	struct t_frame *dst,
	struct keyboard *kbd,
	int32_t lane,
	int32_t x,
	int32_t y,
	int32_t width
) {
	/* make sure we start on on an octave boundary */
	int32_t octave, offset;
	keyboard_lane_decompose(&octave, &offset, lane);

	x     -= offset;
	width += offset;

	/* setup scratch frames */
	struct t_frame frame_scratch = 
		T_SCRATCH_FRAME(128, KBD_OCTAVE_WIDTH, KBD_OCTAVE_HEIGHT);

	while (width > 0) {
		t_frame_init_pattern(&frame_scratch, g_kbd_frame);
		t_frame_paint(&frame_scratch, 
			kbd->color.frame_fg,
			kbd->color.frame_bg
		);
		t_frame_cull(&frame_scratch, ' ');

		for (int32_t j = 0; j < kbd->_tone_pointer; ++j) {
			int32_t tone_octave, tone_note;
			keyboard_index_decompose(
				&tone_octave, &tone_note, kbd->_tones_active[j]._index
			);
			if (tone_octave != octave) {
				continue;
			}

			struct t_frame frame_note = T_SCRATCH_FRAME(128, 0, 0);
			t_frame_init_pattern(&frame_note, g_kbd_notes[tone_note]);
			t_frame_cull(&frame_note, ' ');
			t_frame_overlay(&frame_scratch, &frame_note, 0, 0);
		}

		t_frame_map(&frame_scratch,
			~(0), T_WASHED, kbd->color.idle_white, 'w', ' '
		);
		t_frame_map(&frame_scratch,
			~(0), T_WASHED, kbd->color.idle_black, 'b', ' '
		);
		t_frame_map(&frame_scratch,
			~(0), T_WASHED, kbd->color.active_white, 'W', ' '
		);
		t_frame_map(&frame_scratch,
			~(0), T_WASHED, kbd->color.active_black, 'B', ' '
		);

		t_frame_overlay(dst, &frame_scratch, x, y);

		++octave;
		x     += KBD_OCTAVE_LANES;
		width -= KBD_OCTAVE_LANES;
	}
	return T_OK;
}
