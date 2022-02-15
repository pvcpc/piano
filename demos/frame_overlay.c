#include <terminal.h>
#include <draw.h>
#include <app.h>


int
main()
{
	app_setup_and_never_call_again();

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
	frame_load_pattern(&frame_b, 2, 2,
		"+--+\n"
		"|  |\n"
		"|  |\n"
		"+--+\n"
	);

	frame_stencil_cmp(&frame_a, CELL_CONTENT_BIT, ' ');
	frame_stencil_seteq(&frame_a, CELL_CONTENT_BIT, &CELL_CONTENT(0));

	frame_stencil_cmp(&frame_b, CELL_CONTENT_BIT, ' ');
	frame_stencil_seteq(&frame_b, CELL_CONTENT_BIT, &CELL_CONTENT(0));

	frame_overlay(&frame_a, &frame_b, 0, 0, 0);
	frame_rasterize(&frame_a, 0, 0);

	app_cleanup_and_never_call_again();
	return 0;
}
