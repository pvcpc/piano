#ifndef INCLUDE_KEYBOARD_H
#define INCLUDE_KEYBOARD_H

#include "t_util.h"
#include "t_draw.h"

/* tunable compile-time parameters */
#ifndef KEYBOARD_POLYPHONY
#  define KEYBOARD_POLYPHONY 32
#endif
/* end of tunable compile-time parameters */

#define NOTE_COUNT 12

enum note 
{
	NOTE_C  = 0,
	NOTE_Cs = 1,
	NOTE_Db = 1,
	NOTE_D  = 2,
	NOTE_Ds = 3,
	NOTE_Eb = 3,
	NOTE_E  = 4,
	NOTE_Es = 5,
	NOTE_Fb = 4,
	NOTE_F  = 5,
	NOTE_Fs = 6,
	NOTE_Gb = 6,
	NOTE_G  = 7,
	NOTE_Gs = 8,
	NOTE_Ab = 8,
	NOTE_A  = 9,
	NOTE_As = 10,
	NOTE_Bb = 10,
	NOTE_B  = 11,
};

static inline char const *
note_string(
	enum note note
) {
	switch (note) {
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

/* midi spec: C(-1) = 0, G(9) = 127 */
#define MIDI_INDEX_MAX 127
#define MIDI_INDEX_MASK 0x7f

#define MIDI_INDEX(note, octave) (((octave) + 1) * NOTE_COUNT + (note))
#define INDEX_NOTE(index) (((index) & MIDI_INDEX_MASK) % NOTE_COUNT)
#define INDEX_OCTAVE(index) (((index) & MIDI_INDEX_MASK) / NOTE_COUNT - 1)
#define INDEX_OCTAVE_ZB(index) (((index) & MIDI_INDEX_MASK) / NOTE_COUNT)

struct keyboard_tone
{
	double               _tm_start;
	double               _tm_sustain; /* < 0 indicates infinite sustain */
	uint8_t              _mi;
};

struct keyboard
{
	uint8_t              mi_hi; /* highest visible index on screen */
	uint8_t              mi_lo; /* lowest visible index on screen */

	struct {
		uint32_t         frame_fg;
		uint32_t         frame_bg;

		uint32_t         idle_white;
		uint32_t         idle_black;
		uint32_t         active_white;
		uint32_t         active_black;
	} color;

	struct t_frame       _frame_scratch;
	struct keyboard_tone _tones_active [KEYBOARD_POLYPHONY];
	uint32_t             _tone_pointer;
};

/* use in keyboard_tone_activate when no starting time point is available. */
#define KBD_START_INF -1
/* use in keyboard_tone_activate when sustain should be infinite. */
#define KBD_SUSTAIN_INF -1

enum t_status
keyboard_support_setup();

void
keyboard_support_cleanup();

enum t_status
keyboard_create(
	struct keyboard *kbd
);

void
keyboard_destroy(
	struct keyboard *kbd
);

enum t_status
keyboard_tone_activate(
	struct keyboard *kbd,
	double tm_start,
	double tm_sustain,
	uint8_t mi
);

enum t_status
keyboard_tone_deactivate(
	struct keyboard *kbd,
	uint8_t mi
);

enum t_status
keyboard_tones_deactivate_expired(
	struct keyboard *kbd,
	double tm_point
);

enum t_status
keyboard_draw(
	struct t_frame *dst,
	struct keyboard *kbd,
	int32_t x,
	int32_t y
);

#endif /* INCLUDE_KEYBOARD_H */
