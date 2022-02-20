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
	frame_zero_grid(&frame);
	frame_stencil_setz(&frame, CELL_CONTENT_BIT, &CELL_CONTENT(' '));

	s32 gray_scale [7] = { 36, 72, 108, 144, 180, 216, 252, };

	for (s32 i = 1; i <= 7; ++i) {
		s32 y0 = (frame.height * (i-1)) / 7;
		s32 y1 = (frame.height * i) / 7;
		
		frame_clip_absolute(&frame, 0, y0, frame.width, y1);
		frame_stencil_test(&frame, 0, 0);
		frame_stencil_setz(&frame, CELL_BACKGROUND_BIT, &CELL_BACKGROUND_GRAY(gray_scale[i-1]));
	}

	frame_zero_clip(&frame);

	t_reset();
	t_clear();
	frame_rasterize(&frame, 0, 0);
	t_flush();
	frame_free(&frame);

	app_sleep(3);

	return 0;
}
