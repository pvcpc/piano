#include <stdio.h>
#include <float.h>

#include "t_base.h"
#include "t_sequence.h"
#include "t_draw.h"

#include "keyboard.h"


static char const * const KBD__CH_OCTAVE_FRAME =
	            "+-------------+\n"
	            "||b|b|||b|b|b||\n"
	            "||b|b|||b|b|b||\n"
	            "|+-+-+|+-+-+-+|\n"
	            "|w|w|w|w|w|w|w|\n"
	            "|w|w|w|w|w|w|w|\n"
	            "+-+-+-+-+-+-+-+\n";

static char const *const KBD__CH_OVERLAY_FRAMES [KBD_NOTES] = {
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

struct t_frame g_frame_octave;
struct t_frame g_frame_array_key_overlays [KBD_NOTES];

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
}

enum t_status
keyboard_support_setup()
{
	enum t_status stat;

	/* try allocate frames */
	stat = t_frame_create_pattern(&g_frame_octave, KBD__CH_OCTAVE_FRAME);
	if (stat < 0) {
		fprintf(stderr, "Failed to create keyboard octave t_frame: %s\n",
			t_status_string(stat));
		goto e_fail;
	}

	for (uint32_t i = 0; i < KBD_NOTES; ++i) {
		stat = t_frame_create_pattern(
			&g_frame_array_key_overlays[i], 
			KBD__CH_OVERLAY_FRAMES[i]
		);
		if (stat < 0) {
			fprintf(stderr, "Failed to create frame overlay for %s: %s\n",
				note_string(i), t_status_string(stat));
			goto e_fail;
		}
	}

	/* transform, wash frames clean */
	t_frame_paint(&g_frame_octave, T_WASHED, T_WASHED);
	t_frame_map(&g_frame_octave, T_MAP_CH, 0, 0, ' ', '\0');
	for (uint32_t i = 0; i < KBD_NOTES; ++i) {
		t_frame_paint(&g_frame_array_key_overlays[i], 
			T_WASHED, T_WASHED
		);
		t_frame_map(&g_frame_array_key_overlays[i], T_MAP_CH, 0, 0, ' ', '\0');
	}
	return T_OK;

e_fail:
	keyboard_support_cleanup();
	return stat;
}

void
keyboard_support_cleanup()
{
	t_frame_destroy(&g_frame_octave);
	for (uint32_t i = 0; i < KBD_NOTES; ++i) {
		t_frame_destroy(&g_frame_array_key_overlays[i]);
	}
}

int32_t
keyboard_get_frame_width()
{
	return g_frame_octave.width;
}

int32_t
keyboard_get_frame_height()
{
	return g_frame_octave.height;
}

enum t_status
keyboard_create(
	struct keyboard *kbd
) {
	if (!kbd) return T_ENULL;

	enum t_status stat;
	stat = t_frame_create(&kbd->_frame_scratch, 
		g_frame_octave.width,
		g_frame_octave.height
	);
	if (stat < 0) {
		return stat;
	}
	keyboard__zero(kbd);
	keyboard__defaults(kbd);
	return T_OK;
}

void
keyboard_destroy(
	struct keyboard *kbd
) {
	if (!kbd) return;

	t_frame_destroy(&kbd->_frame_scratch);
	keyboard__zero(kbd);
}

enum t_status
keyboard_tone_activate(
	struct keyboard *kbd,
	double tm_start,
	double tm_sustain,
	int32_t index
) {
	if (!kbd) return T_ENULL;

	/* check if note is already activated */
	for (int32_t i = 0; i < kbd->_tone_pointer; ++i) {
		struct tone *tone = &kbd->_tones_active[i];
		if (tone->_index == index) {
			return T_OK;
		}
	}

	/* it tone is not active, check to make sure we have room */
	if (kbd->_tone_pointer >= KBD_POLYPHONY) return T_EOVERFLO;

	/* otherwise activate it */
	struct tone *tone = 
		&kbd->_tones_active[kbd->_tone_pointer++];
	tone->_tm_start = tm_start;
	tone->_tm_sustain = tm_sustain;
	tone->_index = index;

	return T_OK;
}

enum t_status
keyboard_tone_deactivate(
	struct keyboard *kbd,
	int32_t index 
) {
	if (!kbd) return T_ENULL;

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
	return T_OK;
}

enum t_status
keyboard_tones_deactivate_expired(
	struct keyboard *kbd,
	double tm_point
) {
	if (!kbd) return T_ENULL;

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
	return T_OK;
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
	if (!dst || !kbd) return T_ENULL;

	/* make sure we start on on an octave boundary */
	int32_t octave, offset;
	keyboard_lane_decompose(&octave, &offset, lane);

	x     -= offset;
	width += offset;

	while (width > 0) {
		t_frame_clear(&kbd->_frame_scratch);
		t_frame_overlay(
			&kbd->_frame_scratch, &g_frame_octave, 0, 0
		);
		t_frame_paint(&kbd->_frame_scratch, 
			kbd->color.frame_fg,
			kbd->color.frame_bg
		);

		for (int32_t j = 0; j < kbd->_tone_pointer; ++j) {
			int32_t tone_octave, tone_note;
			keyboard_index_decompose(
				&tone_octave, &tone_note, kbd->_tones_active[j]._index
			);
			if (tone_octave != octave) {
				continue;
			}

			t_frame_overlay(
				&kbd->_frame_scratch, 
				&g_frame_array_key_overlays[tone_note],
				0, 0
			);
		}

		t_frame_map(&kbd->_frame_scratch,
			~(0), T_WASHED, kbd->color.idle_white, 'w', ' '
		);
		t_frame_map(&kbd->_frame_scratch,
			~(0), T_WASHED, kbd->color.idle_black, 'b', ' '
		);
		t_frame_map(&kbd->_frame_scratch,
			~(0), T_WASHED, kbd->color.active_white, 'W', ' '
		);
		t_frame_map(&kbd->_frame_scratch,
			~(0), T_WASHED, kbd->color.active_black, 'B', ' '
		);

		t_frame_overlay(dst, &kbd->_frame_scratch, x, y);

		++octave;
		x     += KBD_OCTAVE_LANES;
		width -= KBD_OCTAVE_LANES;
	}

#if 0
	for (int32_t i = oct_lo; i <= oct_hi; ++i) {
		t_frame_clear(&kbd->_frame_scratch);
		t_frame_blend(&kbd->_frame_scratch, &g_frame_octave,
			~(0), ~(0),
			kbd->color.frame_fg,
			kbd->color.frame_bg,
			0, 0
		);


		t_frame_map_one(&kbd->_frame_scratch,
			~(0), T_WASHED, kbd->color.idle_white, 'w', ' '
		);
		t_frame_map_one(&kbd->_frame_scratch,
			~(0), T_WASHED, kbd->color.idle_black, 'b', ' '
		);
		t_frame_map_one(&kbd->_frame_scratch,
			~(0), T_WASHED, kbd->color.active_white, 'W', ' '
		);
		t_frame_map_one(&kbd->_frame_scratch,
			~(0), T_WASHED, kbd->color.active_black, 'B', ' '
		);

		t_frame_blend(dst, &kbd->_frame_scratch,
			~(0), 0, 0, 0,
			x + (i - oct_lo) * (g_frame_octave.width - 1),
			y
		);
	}
#endif
	return T_OK;
}
