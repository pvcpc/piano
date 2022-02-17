#include <terminal.h>
#include <draw.h>
#include <app.h>


int
demo()
{
	t_reset();
	t_clear();
	t_cursor_pos(1, 1);

	struct frame 
		frame_a = LOCAL_FRAME(6, 6),
		frame_b = LOCAL_FRAME(6, 6);

	frame_zero_grid(&frame_a);
	frame_zero_grid(&frame_b);

	frame_load_pattern(&frame_a, 0, 0,
		"+--+\n"
		"|  |\n"
		"|  |\n"
		"+--+\n"
	);
	frame_load_pattern(&frame_b, 0, 0,
		"+--+\n"
		"|  |\n"
		"|  |\n"
		"+--+\n"
	);

	frame_stencil_cmp(&frame_a, CELL_CONTENT_BIT, ' ');
	frame_stencil_seteq(&frame_a, CELL_CONTENT_BIT, &CELL_CONTENT(0));
	frame_stencil_setne(&frame_a, CELL_BACKGROUND_BIT, &CELL_BACKGROUND_RGB(0, 0, 255));

	frame_stencil_cmp(&frame_b, CELL_CONTENT_BIT, ' ');
	frame_stencil_seteq(&frame_b, CELL_CONTENT_BIT, &CELL_CONTENT(0));

	/* Demonstrate that overlays adhere to clipping */
	struct clip *clip = &frame_a.clip;
	clip->tlx = 3;
	clip->tly = 3;

	frame_overlay(&frame_a, &frame_b, 2, 2, 1);
	frame_stencil_cmp(&frame_a, CELL_STENCIL_BIT, 1);
	frame_stencil_seteq(&frame_a, CELL_BACKGROUND_BIT, &CELL_BACKGROUND_RGB(255, 0, 0));

	clip->tlx = 0;
	clip->tly = 0;

	frame_rasterize(&frame_a, 0, 0);
	return 0;
}
