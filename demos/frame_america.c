#include <terminal.h>
#include <draw.h>
#include <app.h>

int
demo()
{
	struct frame frame = LOCAL_FRAME(128, 128);
	frame_zero_grid(&frame);
	frame_load_pattern(&frame, 0, 0,
		"bbbbbbbbrrrrrrrr\n"
		"bwbwbwbbwwwwwwww\n"
		"bbbbbbbbrrrrrrrr\n"
		"wwwwwwwwwwwwwwww\n"
		"rrrrrrrrrrrrrrrr\n"
		"wwwwwwwwwwwwwwww\n"
	);

	/* blue color */
	frame_stencil_cmp(&frame, CELL_CONTENT_BIT, 'b');
	frame_stencil_seteq(&frame, CELL_BACKGROUND_BIT, &CELL_BACKGROUND_RGB(0, 0, 255));
	frame_stencil_seteq(&frame, CELL_CONTENT_BIT, &CELL_CONTENT(' '));

	/* red color */
	frame_stencil_cmp(&frame, CELL_CONTENT_BIT, 'r');
	frame_stencil_seteq(&frame, CELL_BACKGROUND_BIT, &CELL_BACKGROUND_RGB(255, 0, 0));
	frame_stencil_seteq(&frame, CELL_CONTENT_BIT, &CELL_CONTENT(' '));

	/* white color */
	frame_stencil_cmp(&frame, CELL_CONTENT_BIT, 'w');
	frame_stencil_seteq(&frame, CELL_BACKGROUND_BIT, &CELL_BACKGROUND_RGB(255, 255, 255));
	frame_stencil_seteq(&frame, CELL_CONTENT_BIT, &CELL_CONTENT(' '));

	t_reset();
	t_clear();
	frame_rasterize(&frame, 0, 0);
		
	return 0;
}
