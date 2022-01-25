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
	            "      #        \n"
	            "      #        \n"
	            "               \n",

	[NOTE_Fs] = "               \n"
	            "       #       \n"
	            "       #       \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            "               \n",

	[NOTE_G ] = "               \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            "        #      \n"
	            "        #      \n"
	            "               \n",

	[NOTE_Gs] = "               \n"
	            "         #     \n"
	            "         #     \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            "               \n",

	[NOTE_A ] = "               \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            "          #    \n"
	            "          #    \n"
	            "               \n",

	[NOTE_As] = "               \n"
	            "           #   \n"
	            "           #   \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            "               \n",

	[NOTE_B ] = "               \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            "            #  \n"
	            "            #  \n"
	            "               \n",
};

struct t_frame g_frame_octave;
struct t_frame g_frame_array_key_overlays [NOTE_COUNT];

static void
keyboard__remove_expired_tones(
	struct keyboard *kbd,
	double const tm_now
) {
	for (int32_t i = 0; i < kbd->_tone_pointer; ++i) {
		struct keyboard__tone *tone = &kbd->_tones_active[i];

		if (tone->_tm_activated + tone->_tm_sustain <= tm_now) {
			--kbd->_tone_pointer;
			if (i != kbd->_tone_pointer) { /* swap end with current */
				struct keyboard__tone *repl = 
					&kbd->_tones_active[kbd->_tone_pointer];
				tone->_tm_activated    = repl->_tm_activated;
				tone->_tm_sustain      = repl->_tm_sustain;
				tone->_midi_note_index = repl->_midi_note_index;
			}
			--i; /* don't go to the next elem yet if we swapped this one */
		}
	}
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
		t_frame_paint(&g_frame_array_key_overlays[i], T_WASHED, T_WASHED);
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
keyboard_human_staccato(
	struct keyboard *kbd,
	double tm_sustain,
	uint8_t note_idx
) {
	if (!kbd) return T_ENULL;
	if (note_idx > MIDI_INDEX_MAX) return T_EPARAM;
	if (sustain <= 0) return T_OK;

	double const tm_now = t_elapsed();

	/* check if note is already active */
	for (uint32_t i = 0; i < kbd->_tone_pointer; ++i) {
		struct keyboard__tone *tone = &kbd->_tones_active[i];
		if (tone->_midi_note_index == note_idx) {
			tone->_tm_activated = tm_now;
			tone->_tm_sustain = sustain;
			return T_OK;
		}
	}

	/* if not, try to add it */
	if (kbd->_tone_pointer >= KEYBOARD_POLYPHONY) {
		return T_EOVERFLO;
	}

	struct keyboard__tone *tone = &kbd->_tones_active[kbd->_tone_pointer++];
	tone->_tm_activated = tm_now;
	tone->_tm_sustain = sustain;
	tone->_midi_note_index = note_idx;

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

	double const tm_now = t_elapsed();
	keyboard__remove_expired_tones(kbd, tm_now);

	uint32_t const octave_idx_lo = 
		(MIDI_INDEX_MASK & kbd->midi_index_start) / NOTE_COUNT;
	uint32_t const octave_idx_hi = 
		(MIDI_INDEX_MASK & kbd->midi_index_end) / NOTE_COUNT;

	for (uint32_t i = octave_idx_lo; i <= octave_idx_hi; ++i) {
		x += g_frame_octave.width;
	}
}
