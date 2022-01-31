#include <stdio.h>
#include <float.h>

#include "t_base.h"
#include "t_sequence.h"
#include "t_draw.h"

#include "keyboard.h"


static char const * const KEYBOARD__CH_OCTAVE_FRAME =
	            "+-------------+\n"
	            "||b|b|||b|b|b||\n"
	            "||b|b|||b|b|b||\n"
	            "|+-+-+|+-+-+-+|\n"
	            "|w|w|w|w|w|w|w|\n"
	            "|w|w|w|w|w|w|w|\n"
	            "+-+-+-+-+-+-+-+\n";

static char const *const KEYBOARD__CH_OVERLAY_FRAMES [NOTE_COUNT] = {
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
struct t_frame g_frame_array_key_overlays [NOTE_COUNT];

static inline void
keyboard__zero(
	struct keyboard *kbd
) {
	kbd->mi_hi = 0;
	kbd->mi_lo = 0;

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
	kbd->color.idle_white = T_RGB(224, 224, 224);
	kbd->color.idle_black = T_WASHED;
	kbd->color.active_white = T_RGB( 96,  96,  96);
	kbd->color.active_black = T_RGB(192, 192, 192);
}

enum t_status
keyboard_support_setup()
{
	enum t_status stat;

	/* try allocate frames */
	stat = t_frame_create_pattern(
		&g_frame_octave, 
		 T_FRAME_SPACEHOLDER, 
		 KEYBOARD__CH_OCTAVE_FRAME
	);
	if (stat < 0) {
		fprintf(stderr, "Failed to create keyboard octave t_frame: %s\n",
			t_status_string(stat));
		goto e_fail;
	}

	for (uint32_t i = 0; i < NOTE_COUNT; ++i) {
		stat = t_frame_create_pattern(
			&g_frame_array_key_overlays[i], 
			 T_FRAME_SPACEHOLDER, 
			 KEYBOARD__CH_OVERLAY_FRAMES[i]
		);
		if (stat < 0) {
			fprintf(stderr, "Failed to create frame overlay for %s: %s\n",
				note_string(i), t_status_string(stat));
			goto e_fail;
		}
	}

	/* wash frames clean */
	t_frame_paint(&g_frame_octave, T_WASHED, T_WASHED);
	for (uint32_t i = 0; i < NOTE_COUNT; ++i) {
		t_frame_paint(&g_frame_array_key_overlays[i], 
			T_WASHED, T_WASHED
		);
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
	for (uint32_t i = 0; i < NOTE_COUNT; ++i) {
		t_frame_destroy(&g_frame_array_key_overlays[i]);
	}
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
	double _tm_start,
	double _tm_sustain,
	uint8_t _mi
) {
	if (!kbd) return T_ENULL;
	if (_mi > MIDI_INDEX_MAX) return T_EPARAM;

	/* check if note is already activated */
	for (int32_t i = 0; i < kbd->_tone_pointer; ++i) {
		struct keyboard_tone *tone = &kbd->_tones_active[i];
		if (tone->_mi == _mi) {
			return T_OK;
		}
	}

	/* it tone is not active, check to make sure we have room */
	if (kbd->_tone_pointer >= KEYBOARD_POLYPHONY) return T_EOVERFLO;

	/* otherwise activate it */
	struct keyboard_tone *tone = 
		&kbd->_tones_active[kbd->_tone_pointer++];
	tone->_tm_start = _tm_start;
	tone->_tm_sustain = _tm_sustain;
	tone->_mi = _mi;

	return T_OK;
}

enum t_status
keyboard_tone_deactivate(
	struct keyboard *kbd,
	uint8_t _mi 
) {
	if (!kbd) return T_ENULL;
	if (_mi > MIDI_INDEX_MAX) return T_EPARAM;

	for (int32_t i = 0; i < kbd->_tone_pointer; ++i) {
		struct keyboard_tone *tone = &kbd->_tones_active[i];
		if (tone->_mi != _mi) {
			continue;
		}

		--kbd->_tone_pointer;
		if (i == kbd->_tone_pointer) {
			continue;
		}

		struct keyboard_tone *end = 
			&kbd->_tones_active[kbd->_tone_pointer];
		tone->_tm_start   = end->_tm_start;
		tone->_tm_sustain = end->_tm_sustain;
		tone->_mi         = end->_mi;
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
		struct keyboard_tone *tone = &kbd->_tones_active[i];
		if (tone->_tm_start + tone->_tm_sustain > tm_point) {
			continue;
		}

		--kbd->_tone_pointer;
		if (i == kbd->_tone_pointer) {
			continue;
		}

		struct keyboard_tone *end = 
			&kbd->_tones_active[kbd->_tone_pointer];
		tone->_tm_start   = end->_tm_start;
		tone->_tm_sustain = end->_tm_sustain;
		tone->_mi         = end->_mi;
		--i;
	}
	return T_OK;
}

enum t_status
keyboard_draw(
	struct t_frame *dst,
	struct keyboard *kbd,
	int32_t x,
	int32_t y
) {
	if (!dst || !kbd) return T_ENULL;

	int32_t const 
		oct_lo = INDEX_OCTAVE(kbd->mi_lo),
		oct_hi = INDEX_OCTAVE(kbd->mi_hi);

	for (int32_t i = oct_lo; i <= oct_hi; ++i) {
		t_frame_clear(&kbd->_frame_scratch);
		t_frame_blend(&kbd->_frame_scratch, &g_frame_octave,
			~(0), ~(0),
			kbd->color.frame_fg,
			kbd->color.frame_bg,
			0, 0
		);

		for (int32_t j = 0; j < kbd->_tone_pointer; ++j) {
			struct keyboard_tone *tone = &kbd->_tones_active[j];
			int32_t const tone_oct = INDEX_OCTAVE(tone->_mi);
			int32_t const tone_note = INDEX_NOTE(tone->_mi);
			if (tone_oct != i) {
				continue;
			}
			t_frame_blend(&kbd->_frame_scratch, 
				&g_frame_array_key_overlays[tone_note],
				T_BLEND_CH, 0,
				0, 0,
				0, 0
			);
		}

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
#if 0
			x + i * (g_frame_octave.width - 1), /* -1 to overlap borders */
			y
	}

	for (uint32_t i = 0; i < kbd->_tone_pointer; ++i) {
		struct keyboard_tone *tone = &kbd->_tones_active[i];

		if (tone->_mi < kbd->_mi_lo || kbd->_mi_hi < tone->_mi) {
			/* tone out of range of visual rendering */
			continue;
		}

		int32_t i_oct = INDEX_OCTAVE_ZB(tone->_mi) - oct_lo;
		int32_t i_note = INDEX_NOTE(tone->_mi);

		t_frame_blend(dst, &g_frame_array_key_overlays[i_note],
			~(0), ~(0),
			over_fg_rgb,
			over_bg_rgb,
			x + i_oct * (g_frame_octave.width - 1), /* -1 to overlap borders */
			y
		);
	}
#endif
	return T_OK;
}
