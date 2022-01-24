#include <stdio.h>

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

struct t_frame KEYBOARD__OCTAVE_FRAME;
struct t_frame KEYBOARD__OVERLAY_FRAMES [NOTE_COUNT];

enum t_status
keyboard_support_setup()
{
	enum t_status stat;

	/* try allocate frames */
	stat = t_frame_create_pattern(
		&KEYBOARD__OCTAVE_FRAME, 
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
			&KEYBOARD__OVERLAY_FRAMES[i], 
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
	t_frame_paint(&KEYBOARD__OCTAVE_FRAME, T_WASHED, T_WASHED);
	for (uint32_t i = 0; i < NOTE_COUNT; ++i) {
		t_frame_paint(&KEYBOARD__OVERLAY_FRAMES[i], T_WASHED, T_WASHED);
	}

	return T_OK;

e_fail:
	keyboard_support_cleanup();
	return stat;
}

void
keyboard_support_cleanup()
{
	t_frame_destroy(&KEYBOARD__OCTAVE_FRAME);
	for (uint32_t i = 0; i < NOTE_COUNT; ++i) {
		t_frame_destroy(&KEYBOARD__OVERLAY_FRAMES[i]);
	}
}
