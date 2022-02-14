#include <stdlib.h>

#include "geometry.h"
#include "terminal.h"
#include "draw.h"


static inline u32
direct_write_foreground_256(struct rgba color)
{
	return t_foreground_256(rgb_compress_256(color));
}

static inline u32
direct_write_background_256(struct rgba color) 
{
	return t_background_256(rgb_compress_256(color));
}

struct frame *
frame_realloc(struct frame *frame, s32 width, s32 height)
{
	if (!frame || width < 0 || height < 0) {
		/* @TODO verbose log */
		return NULL;
	}
	
	u32 const requested_size = ALIGN_UP(GRID_SIZEOF(width, height), 16);

	if (requested_size > frame->alloc.grid_alloc_usable_size) {

		struct cell *new_grid;
		if (!(new_grid = realloc(frame->grid, requested_size))) {
			/* @TODO log inconvenience */
			goto e_realloc;
		}

		free(frame->grid);
		frame->grid = new_grid;
		frame->width = width;
		frame->height = height;

		frame->alloc.grid_alloc_base = new_grid;
		frame->alloc.grid_alloc_size = requested_size;
		frame->alloc.grid_alloc_usable_size = requested_size;
	}
	return frame;

e_realloc:
	return NULL;
}

void
frame_free(struct frame *frame)
{
	if (frame) {
		free(frame);
		frame_zero_out(frame);
	}
}

struct frame *
frame_resize(struct frame *frame, s32 width, s32 height)
{
	if (!frame || width < 0 || height < 0) {
		/* @TODO verbose log */
		return NULL;
	}

	/* client-code might not want this alignment, only useful for auto managed frame
	u32 const requested_size = ALIGN_UP(GRID_SIZEOF(width, height), 16);
	*/

	u32 const requested_size = GRID_SIZEOF(width, height);

	if (requested_size > frame->alloc.grid_alloc_usable_size) {
		/* @TODO verbose logging */
		return NULL;
	}

	frame->width = width;
	frame->height = height;

	return frame;
}

u32
frame_load_pattern(struct frame *frame, s32 x, s32 y, char const *pattern)
{
	if (!frame || !pattern) {
		/* @TODO log verbosely */
		return 0;
	}

	u32 num_emplaced = 0;
	s32 i = x,
		j = y;

	while (*pattern) {
		switch (*pattern) { /* sure we can cascade, but that's mental overhead */

		case '\n':
			i = x;
			++j;
			break;

		case '\v':
			++j;
			break;

		case '\r':
			i = x;
			break;

		default:
			if ( (0 <= i && i < frame->width) &&
				 (0 <= j && j < frame->height) )
			{
				frame_cell_at(frame, i, j)->content = *pattern;
				++num_emplaced;
				++i;
			}
		}
		++pattern;
	}
	return num_emplaced;
}

u32
frame_rasterize(struct frame *frame, s32 x, s32 y)
{
	s32 term_w, term_h;
	t_query_size(&term_w, &term_h);

	struct box dstbox, srcbox;
	box_intersect_with_offset(
		&dstbox, &srcbox,
		&BOX_SCREEN(term_w, term_h),
		&BOX(
			frame->clip.tlx, frame->clip.tly,
			frame->width + frame->clip.brx, 
			frame->height + frame->clip.bry
		),
		x, y
	);

	s32 const 
		true_w = BOX_WIDTH(&dstbox),
		true_h = BOX_HEIGHT(&dstbox);

	/* How much was actually written out to the terminal in bytes, we're
	 * mostly interested if this remains 0. @TODO maybe make this part 
	 * of the actual terminal interface because keeping track of these 
	 * additions is annoying. */
	u32 throughput = 0;

	/* any constants less than -1 required for init position to
	 * push through initial positions onto the terminal */
	s32 prior_x = INT16_MIN;
	s32 prior_y = INT16_MIN;

	struct rgba prior_fg = RGBA(0, 0, 0, 0);
	struct rgba prior_bg = RGBA(0, 0, 0, 0);
	throughput += t_reset();

	for (s32 j = 0; j < true_h; ++j) {
		for (s32 i = 0; i < true_w; ++i) {
			s32 const
				src_x = srcbox.x0 + i,
				src_y = srcbox.y0 + j;

			struct cell *cell = frame_cell_at(frame, src_x, src_y);
			if (!cell->content) {
				continue;
			}

			int32_t const
				dst_x = dstbox.x0 + i,
				dst_y = dstbox.y0 + j;

			/* CURSOR POS */
			s32 delta_x = dst_x - prior_x;
			s32 delta_y = dst_y - prior_y;

			/* @SPEED(max): take a look into this a bit more */
			/* @NOTE(max): converting delta_y into \v chars does NOT
			 * improve throughput significantly.
			 */
			/* minimaly optimize byte usage for relocation */
			if (delta_x && delta_y) {
				t_cursor_pos(dst_x + 1, dst_y + 1);
			}
			else if (delta_x > 0) {
				t_cursor_forward(delta_x);
			}
			else if (delta_x < 0) {
				t_cursor_back(-delta_x);
			}
			else if (delta_y > 0) {
				t_cursor_down(delta_y);
			}
			else if (delta_y < 0) {
				t_cursor_up(delta_y);
			}
			prior_x = dst_x + 1; /* to account for cursor advancing right when writing */
			prior_y = dst_y;
			
			/* COLOR */
			if (rgba_not_equal(cell->fg, prior_fg) ||
				rgba_not_equal(cell->bg, prior_bg))
			{
				if (!cell->fg.a || !cell->bg.a) {
					throughput += t_reset();
					prior_fg = RGBA(0, 0, 0, 0);
					prior_bg = RGBA(0, 0, 0, 0);
				}
				if (rgba_not_equal(cell->fg, prior_fg)) {
					prior_fg = cell->fg;
					direct_write_foreground_256(cell->fg);
				}
				if (rgba_not_equal(cell->bg, prior_bg)) {
					prior_bg = cell->bg;
					direct_write_background_256(cell->bg);
				}
			}

			throughput += t_writec(cell->content);
		}
	}
	return throughput;
}
