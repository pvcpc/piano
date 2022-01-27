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

struct keyboard__tone
{
	/* internal */
	double                _tm_activated;
	double                _tm_sustain;
	uint8_t               _midi_note_index;
};

struct keyboard
{
	uint8_t               midi_index_start;
	uint8_t               midi_index_end;

	/* internal */
	struct keyboard__tone _tones_active [KEYBOARD_POLYPHONY];
	uint32_t              _tone_pointer;
};

enum t_status
keyboard_support_setup();

void
keyboard_support_cleanup();

enum t_status
keyboard_human_staccato(
	struct keyboard *kbd,
	double tm_sustain,
	uint8_t note_idx
);

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
);

#endif /* INCLUDE_KEYBOARD_H */
