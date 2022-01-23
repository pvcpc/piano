#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

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

static char const EMPTY_FRAME [] =
	"               \n"
	"               \n"
	"               \n"
	"               \n"
	"               \n"
	"               \n"
	"               \n";

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


struct t_frame framebuffer;


int
main(void)
{
	t_setup();

	t_frame_create(&framebuffer, 0, 0);

	while (1) {

		struct t_frame frame;
		t_frame_create_pattern(&frame, 0, OCTAVE_FRAME);
		t_frame_paint(&frame, T_WASHED, T_WASHED);
		frame.grid[0].fg_rgb = T_RGB(0, 0, 0);
		frame.grid[0].bg_rgb = T_RGB(255, 255, 255);
		t_frame_rasterize(&frame, 0, 0);
		t_frame_destroy(&frame);
		t_flush();
		t_sleep(1e-3);

		switch (t_poll()) {
		}
	}

	t_cleanup();
	return 0;
}
