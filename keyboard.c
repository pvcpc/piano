#include <stdio.h>
#include <float.h>

#include "t_base.h"
#include "t_sequence.h"
#include "t_draw.h"

#include "keyboard.h"


static char const * const KEYBOARD__CH_OCTAVE_FRAME =
	            "+-------------+\n"
	            "|| | ||| | | ||\n"
	            "|| | ||| | | ||\n"
	            "|+-+-+|+-+-+-+|\n"
	            "| | | | | | | |\n"
	            "| | | | | | | |\n"
	            "+-+-+-+-+-+-+-+\n";

static char const *const KEYBOARD__CH_OVERLAY_FRAMES [NOTE_COUNT] = {
	[NOTE_C ] = "               \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            " #             \n"
	            " #             \n"
	            "               \n",

	[NOTE_Cs] = "               \n"
	            "  #            \n"
	            "  #            \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            "               \n",

	[NOTE_D ] = "               \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            "   #           \n"
	            "   #           \n"
	            "               \n",

	[NOTE_Ds] = "               \n"
	            "    #          \n"
	            "    #          \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            "               \n",

	[NOTE_E ] = "               \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            "     #         \n"
	            "     #         \n"
	            "               \n",

	[NOTE_F ] = "               \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            "       #       \n"
	            "       #       \n"
	            "               \n",

	[NOTE_Fs] = "               \n"
	            "        #      \n"
	            "        #      \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            "               \n",

	[NOTE_G ] = "               \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            "         #     \n"
	            "         #     \n"
	            "               \n",

	[NOTE_Gs] = "               \n"
	            "          #    \n"
	            "          #    \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            "               \n",

	[NOTE_A ] = "               \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            "           #   \n"
	            "           #   \n"
	            "               \n",

	[NOTE_As] = "               \n"
	            "            #  \n"
	            "            #  \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            "               \n",

	[NOTE_B ] = "               \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            "             # \n"
	            "             # \n"
	            "               \n",
};

struct t_frame g_frame_octave;
struct t_frame g_frame_array_key_overlays [NOTE_COUNT];

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
keyboard_tone_activate(
	struct keyboard *kbd,
	double tm_start,
	double tm_sustain,
	uint8_t mi
) {
	if (!kbd) return T_ENULL;
	if (mi > MIDI_INDEX_MAX) return T_EPARAM;

	/* check if note is already activated */
	for (int32_t i = 0; i < kbd->ro.tone_pointer; ++i) {
		struct keyboard_tone *tone = &kbd->ro.tones_active[i];
		if (tone->ro.mi == mi) {
			return T_OK;
		}
	}

	/* it tone is not active, check to make sure we have room */
	if (kbd->ro.tone_pointer >= KEYBOARD_POLYPHONY) return T_EOVERFLO;

	/* otherwise activate it */
	struct keyboard_tone *tone = 
		&kbd->ro.tones_active[kbd->ro.tone_pointer++];
	tone->ro.tm_start = tm_start;
	tone->ro.tm_sustain = tm_sustain;
	tone->ro.mi = mi;

	return T_OK;
}

enum t_status
keyboard_tone_deactivate(
	struct keyboard *kbd,
	uint8_t mi 
) {
	if (!kbd) return T_ENULL;
	if (mi > MIDI_INDEX_MAX) return T_EPARAM;

	for (int32_t i = 0; i < kbd->ro.tone_pointer; ++i) {
		struct keyboard_tone *tone = &kbd->ro.tones_active[i];
		if (tone->ro.mi != mi) {
			continue;
		}

		--kbd->ro.tone_pointer;
		if (i == kbd->ro.tone_pointer) {
			continue;
		}

		struct keyboard_tone *end = 
			&kbd->ro.tones_active[kbd->ro.tone_pointer];
		tone->ro.tm_start   = end->ro.tm_start;
		tone->ro.tm_sustain = end->ro.tm_sustain;
		tone->ro.mi         = end->ro.mi;
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

	for (int32_t i = 0; i < kbd->ro.tone_pointer; ++i) {
		struct keyboard_tone *tone = &kbd->ro.tones_active[i];
		if (tone->ro.tm_start + tone->ro.tm_sustain > tm_point) {
			continue;
		}

		--kbd->ro.tone_pointer;
		if (i == kbd->ro.tone_pointer) {
			continue;
		}

		struct keyboard_tone *end = 
			&kbd->ro.tones_active[kbd->ro.tone_pointer];
		tone->ro.tm_start   = end->ro.tm_start;
		tone->ro.tm_sustain = end->ro.tm_sustain;
		tone->ro.mi         = end->ro.mi;
		--i;
	}
	return T_OK;
}

enum t_status
keyboard_draw(
	struct t_frame *dst,
	struct keyboard *kbd,
	int32_t frame_fg_rgb,
	int32_t frame_bg_rgb,
	int32_t over_fg_rgb,
	int32_t over_bg_rgb,
	int32_t x,
	int32_t y
) {
	if (!dst || !kbd) return T_ENULL;

	int32_t const 
		oct_lo = INDEX_OCTAVE_ZB(kbd->mi_lo),
		oct_hi = INDEX_OCTAVE_ZB(kbd->mi_hi);

	for (int32_t i = 0; i <= (oct_hi - oct_lo); ++i) {
		t_frame_blend(dst, &g_frame_octave,
			T_BLEND_ALL,
			frame_fg_rgb,
			frame_bg_rgb,
			x + i * (g_frame_octave.width - 1), /* -1 to overlap borders */
			y
		);
	}

	for (uint32_t i = 0; i < kbd->ro.tone_pointer; ++i) {
		struct keyboard_tone *tone = &kbd->ro.tones_active[i];

		if (tone->ro.mi < kbd->mi_lo || kbd->mi_hi < tone->ro.mi) {
			/* tone out of range of visual rendering */
			continue;
		}

		int32_t i_oct = INDEX_OCTAVE_ZB(tone->ro.mi) - oct_lo;
		int32_t i_note = INDEX_NOTE(tone->ro.mi);

		t_frame_blend(dst, &g_frame_array_key_overlays[i_note],
			T_BLEND_ALL, 
			over_fg_rgb,
			over_bg_rgb,
			x + i_oct * (g_frame_octave.width - 1), /* -1 to overlap borders */
			y
		);
	}
	return T_OK;
}
