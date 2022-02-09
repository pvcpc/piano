#ifndef INCLUDE_KEYBOARD_H
#define INCLUDE_KEYBOARD_H

#include <stdbool.h>

#include "t_util.h"
#include "t_draw.h"

/* @TUNABLE compile-time constants (see README):
 * - KBD_POLYPHONY (default 32):
 *   Set the maximum number of tones a keyboard can activate.
 */
#ifndef KBD_POLYPHONY
#  define KBD_POLYPHONY 32
#endif

#define KBD_OCTAVE_WIDTH 15
#define KBD_OCTAVE_HEIGHT 7
#define KBD_OCTAVE_LANES (KBD_OCTAVE_WIDTH - 1)

#define KBD_NOTES 12

enum note 
{
	NOTE_INVALID = -1,
	NOTE_C       = 0,
	NOTE_Cs      = 1,
	NOTE_Db      = 1,
	NOTE_D       = 2,
	NOTE_Ds      = 3,
	NOTE_Eb      = 3,
	NOTE_E       = 4,
	NOTE_Es      = 5,
	NOTE_Fb      = 4,
	NOTE_F       = 5,
	NOTE_Fs      = 6,
	NOTE_Gb      = 6,
	NOTE_G       = 7,
	NOTE_Gs      = 8,
	NOTE_Ab      = 8,
	NOTE_A       = 9,
	NOTE_As      = 10,
	NOTE_Bb      = 10,
	NOTE_B       = 11,
};

static inline char const *
note_string(
	enum note note
) {
	switch (note) {
	case NOTE_INVALID:
		return "INVALID";
	case NOTE_C:
		return "C";
	case NOTE_Cs:
		return "C#";
	case NOTE_D:
		return "D";
	case NOTE_Ds:
		return "D#";
	case NOTE_E:
		return "E";
	case NOTE_F:
		return "F";
	case NOTE_Fs:
		return "F#";
	case NOTE_G:
		return "G";
	case NOTE_Gs:
		return "G#";
	case NOTE_A:
		return "A";
	case NOTE_As:
		return "A#";
	case NOTE_B:
		return "B";
	default:
		return "undefined_note";
	}
}

static inline void
roll_lane_decompose_with_note(
	int32_t *out_octave,
	int32_t *out_offset,
	enum note *out_note,
	int32_t lane
) {
	static enum note const l_offset_note_table[KBD_OCTAVE_LANES] = {
		NOTE_INVALID,
		NOTE_C,
		NOTE_Cs,
		NOTE_D,
		NOTE_Ds,
		NOTE_E,
		NOTE_INVALID,
		NOTE_F,
		NOTE_Fs,
		NOTE_G,
		NOTE_Gs,
		NOTE_A,
		NOTE_As,
		NOTE_B,
	};

	int32_t oct, off;
	t_mdiv32(&oct, &off, lane, KBD_OCTAVE_LANES);

	*out_octave = oct-1;
	*out_offset = off;
	*out_note = l_offset_note_table[*out_offset];
}

static inline void
roll_lane_decompose(
	int32_t *out_octave,
	int32_t *out_offset,
	int32_t lane
) {
	int32_t oct, off;
	t_mdiv32(&oct, &off, lane, KBD_OCTAVE_LANES);

	*out_octave = oct-1;
	*out_offset = off;
}

static inline int32_t
roll_lane_compose_with_note(
	int32_t octave,
	enum note note
) {
	static int32_t const l_note_offset_table[KBD_NOTES] = {
		1, 2, 3, 4, 5,
		7, 8, 9, 10, 11, 12,
	};
	return KBD_OCTAVE_LANES * (octave + 1) + l_note_offset_table[note % KBD_NOTES];
}

static inline int32_t
roll_lane_compose(
	int32_t octave,
	int32_t offset
) {
	return KBD_OCTAVE_LANES * (octave + 1) + (offset % KBD_OCTAVE_LANES);
}

static inline void
roll_index_decompose(
	int32_t *out_octave,
	enum note *out_note,
	int32_t index
) {
	int32_t oct, note;
	t_mdiv32(&oct, &note, index, KBD_NOTES);

	*out_octave = oct-1;
	*out_note = note;
}

static inline int32_t
roll_index_compose(
	int32_t octave,
	enum note note
) {
	return KBD_NOTES * (octave + 1) + (note % KBD_NOTES);
}

/* +--- MIDI ROLL -------------------------------------------------+ */
struct roll 
{
	int32_t view_lane;
	int32_t view_tick;
	int32_t view_tickscale;

	int32_t cursor_index;
	int32_t cursor_atom;

	int32_t _indices[KBD_POLYPHONY];
	int32_t _indices_ptr;
};

void
roll_init(
	struct roll *roll
);

void
roll_tone_activate(
	struct roll *roll,
	int32_t index
);

void
roll_tone_deactivate(
	struct roll *roll,
	int32_t index
);

void
roll_tone_toggle(
	struct roll *roll,
	int32_t index
);

enum t_status
roll_draw(
	struct t_frame *dst,
	struct roll *roll,
	int32_t width,
	int32_t height,
	int32_t x,
	int32_t y
);

#endif /* INCLUDE_KEYBOARD_H */
