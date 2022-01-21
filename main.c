#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <fcntl.h>
#include <unistd.h>

#include "t_base.h"
#include "t_sequence.h"
#include "t_draw.h"


/* virtual keyboard logic */
enum keyboard_note
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

#define KEYBOARD_OCTAVE_NOTES 12
#define KEYBOARD_OCTAVES(Nm) (Nm * KEYBOARD_OCTAVE_NOTES)
#define KEYBOARD_ADDRESS(Octave, Note) (Octave * KEYBOARD_NUM_OCTAVE_NOTES + Note)

static char const OCTAVE_FRAME [] =
	"+-------------+\n"
	"|| | ||| | | ||\n"
	"|| | ||| | | ||\n"
	"|+-+-+|+-+-+-+|\n"
	"| | | | | | | |\n"
	"| | | | | | | |\n"
	"+-+-+-+-+-+-+-+\n";

static char const *const KEY_OVERLAYS [] = {
	[NOTE_C ] = "               \n"
	            "               \n"
	            "               \n"
	            "               \n"
	            " #             \n"
	            " #             \n"
	            "               \n",
	[NOTE_Cs] = "               \n"
	            "               \n"
	            "  #            \n"
	            "  #            \n"
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
	            "               \n"
	            "    #          \n"
	            "    #          \n"
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
	            "               \n"
	            "       #       \n"
	            "       #       \n"
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
	            "               \n"
	            "         #     \n"
	            "         #     \n"
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
	            "               \n"
	            "           #   \n"
	            "           #   \n"
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


int
main(void)
{
	t_setup();

	double tm_old = t_elapsed();
	while (1) {
		double tm_now = t_elapsed();
		char ups_string [32] = {0};
		snprintf(ups_string, 31, "UPS: %.1f Hz", 1.0 / (tm_now - tm_old));

		int32_t poll_code = t_poll();

		int32_t vp_width, vp_height;
		t_termsize(&vp_width, &vp_height);

		t_reset();
		t_clear();

		struct t_frame frame;
		t_frame_create_pattern(
			&frame, OCTAVE_FRAME, T_FRAME_SPACEHOLDER
		);
		t_frame_paint(&frame, T_RGB(0, 0, 0), T_RGB(255, 255, 255));
		t_frame_rasterize(&frame, 0, 0);
		t_frame_destroy(&frame);

		t_cursor_pos(1, 1);
		t_write_z(ups_string);

		t_flush();

		switch (poll_code) {
		case T_POLL_CODE(0, 'h'):
			puts("h was pressed");
			break;
		case T_POLL_CODE(0, 'j'):
			puts("j was pressed");
			break;
		case T_POLL_CODE(0, 'k'):
			puts("k was pressed");
			break;
		case T_POLL_CODE(0, 'l'):
			puts("l was pressed");
			break;
		}
		tm_old = tm_now;
	}

	t_cleanup();
	return 0;
}
