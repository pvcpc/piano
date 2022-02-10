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

void 
roll_init(
	struct roll *roll
) {
	roll->view_lane = 0;
	roll->view_tick = 0;
	roll->view_tickscale = 1;

	roll->cursor_index = 0;
	roll->cursor_atom = 0;

	roll->_indices_ptr = 0;
}

void
roll_tone_activate(
	struct roll *roll,
	int32_t index
) {
	if (roll->_indices_ptr >= KBD_POLYPHONY) return;

	for (uint32_t i = 0; i < roll->_indices_ptr; ++i) {
		if (roll->_indices[i] == index) return;
	}
	roll->_indices[roll->_indices_ptr++] = index;
}

void
roll_tone_deactivate(
	struct roll *roll,
	int32_t index 
) {
	for (int32_t i = 0; i < roll->_indices_ptr; ++i) {
		if (roll->_indices[i] != index) continue;

		--roll->_indices_ptr;
		if (i == roll->_indices_ptr) {
			continue;
		}

		T_SWAP(int32_t,
			roll->_indices[i],
			roll->_indices[roll->_indices_ptr]
		);
	}
}

void
roll_tone_toggle(
	struct roll *roll,
	int32_t index
) {
	
}

enum t_status
roll_draw(
	struct t_frame *dst,
	struct roll *roll
) {
	/* start on an octave boundary */
	int32_t octave, offset;
	roll_lane_decompose(&octave, &offset, roll->view_lane);

	struct t_box clip;
	t_frame_compute_clip(&clip, dst);

	int32_t const
		x = clip.x0, 
		y = clip.y0, 
		width = T_BOX_WIDTH(&clip), 
		height = T_BOX_HEIGHT(&clip);

	int32_t const
		align_x = x - offset,
		align_w = width + offset;

	/* compute layout */
	int32_t const
		roll_x = x,
		roll_y = y,
		roll_width = width,
		roll_height = height - KBD_OCTAVE_HEIGHT;

	int32_t const
		kbd_x = align_x,
		kbd_y = height - KBD_OCTAVE_HEIGHT;
	
	/* roll lanes */
	{
		int32_t view_lane_end = roll->view_lane + roll_width;

		for (int32_t lane = roll->view_lane; lane < view_lane_end; ++lane) {
			int32_t octave, offset;
			enum note note;
			roll_lane_decompose_with_note(&octave, &offset, &note, lane);
			if (note == NOTE_INVALID) {
				continue;
			}

			int32_t const lane_x = lane + roll_x;
			for (int32_t i = 0; i < roll_height; ++i) {
				int32_t const lane_y = i + roll_y;
				t_frame_cell_at(dst, lane_x, lane_y)->ch = '|';
			}
		}
	}

	/* keyboard */
	{
		struct t_frame frame_scratch = 
			T_SCRATCH_FRAME(128, KBD_OCTAVE_WIDTH, KBD_OCTAVE_HEIGHT);

		int32_t width_covered = 0;
		while (width_covered < align_w) {
			t_frame_init_pattern(&frame_scratch, g_kbd_frame);
			t_frame_cull(&frame_scratch, ' ');

			for (int32_t j = 0; j < roll->_indices_ptr; ++j) {
				int32_t tone_octave, tone_note;
				roll_index_decompose(
					&tone_octave, &tone_note, roll->_indices[j]
				);
				if (tone_octave != octave) {
					continue;
				}

				struct t_frame frame_note = T_SCRATCH_FRAME(128, 0, 0);
				t_frame_init_pattern(&frame_note, g_kbd_notes[tone_note]);
				t_frame_cull(&frame_note, ' ');
				t_frame_overlay(&frame_scratch, &frame_note, 0, 0);
			}

			t_frame_overlay(dst, &frame_scratch, 
				kbd_x + width_covered, kbd_y
			);

			++octave;
			width_covered += KBD_OCTAVE_LANES;
		}
	}
	
	return T_OK;
}
