#include <terminal.h>
#include <draw.h>
#include <app.h>


#define FIBCNT 10

int
demo()
{
	s32 term_w, term_h;
	t_query_size(&term_w, &term_h);

	struct frame frame;
	frame_alloc(&frame, term_w, term_h);
	frame_zero_grid(&frame);
	frame_stencil_setz(&frame, CELL_CONTENT_BIT, &CELL_CONTENT(' '));

	s32 colors [FIBCNT] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, };

	s32 fib [FIBCNT] = { 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, };
	s32 x_space [FIBCNT];
	s32 y_space [FIBCNT];

	s32 tlx = 0,
		tly = 0,
		brx = 0,
		bry = 0;

	/* If we want the largest block to always be on the left, we need
	 * to determine which direction to start placing blocks in. 
	 *
	 * `FIBCNT-1` is required here instead of `FIBCNT` because `i` in
	 * the loop below only goes up to `FIBCNT-1`.
	 */
	s32 const dir_offset = (4 - ((FIBCNT-1) % 4)) % 4;

	/* generate the fibonacci tiling in a virtual space that's easy to
	 * work with (where the first block is at (0, 0)) */
	for (s32 i = 0; i < FIBCNT; ++i) {
		
		/* The order in which the directions are placed in the case
		 * blocks is arbitrary and should be determined by the application
		 * needs. In our case, we want the largest block to always be
		 * on the left-side (no matter how many total blocks there are)
		 * so it makes sense to have 0 be `left` to simplify modular
		 * arithmetic above. */
		switch ((i + dir_offset) % 4) {
		case 0: /* left */
			x_space[i] = tlx - fib[i];
			y_space[i] = tly;
			bry = tly + fib[i];
			tlx -= fib[i];
			break;
		case 1: /* down */
			x_space[i] = tlx;
			y_space[i] = bry;
			brx = tlx + fib[i];
			bry += fib[i];
			break;
		case 2: /* right */
			x_space[i] = brx;
			y_space[i] = tly;
			bry = tly + fib[i];
			brx += fib[i];
			break;
		case 3: /* up */
			x_space[i] = tlx;
			y_space[i] = tly - fib[i];
			brx = tlx + fib[i];
			tly -= fib[i];
			break;
		}
	}

	/* Translate the fibonacci tiling space so that (tlx, tly) is (0, 0) */
	for (s32 i = 0; i < FIBCNT; ++i) {
		x_space[i] -= tlx;
		y_space[i] -= tly;
	}
	brx -= tlx;
	bry -= tly;
	tlx = 0;
	tly = 0;

	/* Scale the translated fibonacci space into viewport coordinates */
	s32 const fib_width = brx - tlx;
	s32 const fib_height = bry - tly;

	for (s32 i = 0; i < FIBCNT; ++i) {
		s32 const x0 = (frame.width * x_space[i]) / fib_width;
		s32 const y0 = (frame.height * y_space[i]) / fib_height;

		s32 const x1 = (frame.width * (x_space[i] + fib[i])) / fib_width;
		s32 const y1 = (frame.height * (y_space[i] + fib[i])) / fib_height;

		frame_clip_absolute(&frame, x0, y0, x1, y1);
		frame_stencil_test(&frame, 0, 0);
		frame_stencil_setz(&frame, CELL_BACKGROUND_BIT, &CELL_BACKGROUND(colors[i]));
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
