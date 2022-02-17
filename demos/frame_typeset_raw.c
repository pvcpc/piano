#include <terminal.h>
#include <draw.h>
#include <app.h>

int
demo()
{
	s32 term_w, term_h;
	t_query_size(&term_w, &term_h);

	struct frame frame;
	frame_alloc(&frame, term_w, term_h);

	struct inset *clip = &frame.clip;
	frame_clip_absolute(&frame, 2, 2, 8, 8);

	frame_typeset_raw(&frame, 0, 0, 1, "Hello, world!");
	frame_stencil_cmp(&frame, CELL_STENCIL_BIT, 1);
	frame_stencil_seteq(&frame, CELL_BACKGROUND_BIT, &CELL_BACKGROUND(rgb256(255, 0, 0)));

	frame_typeset_raw(&frame, 5, 5, 2, "This is a certified hoooood classic!");
	frame_stencil_cmp(&frame, CELL_STENCIL_BIT, 2);
	frame_stencil_seteq(&frame, CELL_BACKGROUND_BIT, &CELL_BACKGROUND(rgb256(0, 255, 0)));

	frame_typeset_raw(&frame, 1, 3, 3,
		"Yes we support multi-line\n"
		"typsetting, how could yo-\n"
		"tell?\n"
	);
	frame_stencil_cmp(&frame, CELL_STENCIL_BIT, 3);
	frame_stencil_seteq(&frame, CELL_BACKGROUND_BIT, &CELL_BACKGROUND(rgb256(0, 0, 255)));

	frame.clip = *clip;

	t_reset();
	t_clear();
	frame_rasterize(&frame, 0, 0);
	t_flush();

	t_cursor_pos(0, 10);
	return 0;
}
