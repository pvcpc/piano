#include <stdlib.h>
#include <string.h>

#include "t_base.h"
#include "t_sequence.h"

#include "t_draw.h"


static inline struct t_cell *
t__frame_cell_at(
	struct t_frame *frame,
	uint32_t x,
	uint32_t y
) {
	return &frame->grid[(y * frame->_true_width) + x];
}

enum t_status
t_frame_create(
	struct t_frame *out,
	uint32_t width,
	uint32_t height
) {
	if (!out) return T_ENULL;

	out->grid = NULL;
	out->width = 0;
	out->height = 0;

	out->_true_width = 0;
	out->_true_height = 0;

	return t_frame_resize(out, width, height);
}

enum t_status
t_frame_create_pattern(
	struct t_frame *out,
	enum t_frame_flag flags,
	char const *pattern
) {
	if (!out || !pattern) return T_ENULL;

	char const *iter = pattern;
	
	/* @NOTE(max): since height = 0, there must be a \n to commit
	 * whatever line is processed for a pattern. 
	 */
	uint32_t width = 0;
	uint32_t height = 0;

	/* incremented width of the currently scanned char row */
	uint32_t row_width = 0;

	while (*iter) {
		switch (*iter) {
		case '\n':
			++height;
		case '\r':
			row_width = 0;
			break;
		case '\v':
			++height;
			break;
		default:
			++row_width;
			break;
		}
		width = T_MAX(width, row_width);
		++iter;
	}

	/* try to create the frame */
	enum t_status create_stat;
	if ((create_stat = t_frame_create(out, width, height)) < 0) {
		return create_stat;
	}

	/* copy the pattern into the frame */
	iter = pattern;

	uint32_t x = 0;
	uint32_t y = 0;

	while (*iter) {
		switch (*iter) {
		case '\n':
			++y;
		case '\r':
			x = 0;
			break;
		case '\v':
			++y;
			break;
		default:
			if (*iter != ' ' || !(flags & T_FRAME_SPACEHOLDER)) {
				t__frame_cell_at(out, x, y)->ch = *iter;
			}
			++x;
			break;
		}
		++iter;
	}

	return T_OK;
}

void
t_frame_destroy(
	struct t_frame *frame
) {
	if (!frame) return;

	free((void *) frame->grid);
}

enum t_status
t_frame_resize(
	struct t_frame *dst,
	uint32_t n_width,
	uint32_t n_height
) {
	if (!dst) return T_ENULL;
	if (!n_width || !n_height) return T_OK;

	uint32_t const n_true_width = T_ALIGN_UP(n_width, TC_CELL_BLOCK_WIDTH);
	uint32_t const n_true_height = T_ALIGN_UP(n_height, TC_CELL_BLOCK_HEIGHT);

	uint32_t const old_true_bufsz = 
		sizeof(struct t_cell) * dst->_true_width * dst->_true_height;
	uint32_t const new_true_bufsz = 
		sizeof(struct t_cell) * n_true_width * n_true_height;

	if (old_true_bufsz < new_true_bufsz) {
		dst->grid = realloc(dst->grid, new_true_bufsz);
		if (!dst->grid) {
			return T_EMALLOC;
		}
		dst->width = n_width;
		dst->height = n_height;

		dst->_true_width = n_true_width;
		dst->_true_height = n_true_height;
	}
	return T_OK;
}

enum t_status
t_frame_clear(
	struct t_frame *frame
) {
	if (!frame) return T_ENULL;

	memset(frame->grid, 0,
		(sizeof(struct t_cell) * 
		 frame->_true_width    * 
		 frame->_true_height)
	);

	return T_OK;
}

enum t_status
t_frame_paint(
	struct t_frame *dst,
	int fg_rgb,
	int bg_rgb
) {
	if (!dst) return T_ENULL;

	for (uint32_t j = 0; j < dst->height; ++j) {
		for (uint32_t i = 0; i < dst->width; ++i) {
			struct t_cell *cell = t__frame_cell_at(dst, i, j);
			cell->fg_rgb = fg_rgb;
			cell->bg_rgb = bg_rgb;
		}
	}
	return T_OK;
}

enum t_status
t_frame_blend(
	struct t_frame *dst,
	struct t_frame *src,
	int32_t x,
	int32_t y
) {
	return T_OK;
}

enum t_status
t_frame_rasterize(
	struct t_frame *frame,
	int32_t x,
	int32_t y
) {
	if (!frame) return T_ENULL;

	int32_t vp_width, vp_height;
	t_termsize(&vp_width, &vp_height);

	int32_t
		bb_x0 = T_MAX(0, x),
		bb_y0 = T_MAX(0, y),
		bb_x1 = T_MIN(vp_width,  x + frame->width),
		bb_y1 = T_MIN(vp_height, y + frame->height);

	/* any constants less than -1 required for init */
	int32_t prior_x = -65535;
	int32_t prior_y = -65535;

	int32_t prior_fg = -1;
	int32_t prior_bg = -1;

	for (uint32_t bb_y = bb_y0; bb_y < bb_y1; ++bb_y) {
		for (uint32_t bb_x = bb_x0; bb_x < bb_x1; ++bb_x) {

			struct t_cell *cell = t__frame_cell_at(frame, bb_x - x, bb_y - y);
			if (!cell->ch) {
				continue;
			}

			/* CURSOR POS */
			/* "bb_x-1" to account for character advancing right. */
			int32_t delta_x = (bb_x-1) - prior_x;
			int32_t delta_y = bb_y - prior_y;

			/* @SPEED(max): take a look into this a bit more */
			/* @NOTE(max): converting delta_y into \v chars does NOT
			 * improve throughput significantly.
			 */
			/* minimaly optimize byte usage for relocation */
			if (delta_x && delta_y) {
				t_cursor_pos(bb_x + 1, bb_y + 1);
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
			prior_x = bb_x;
			prior_y = bb_y;
			
			/* COLOR */
			if (cell->fg_rgb < 0 || cell->bg_rgb < 0) {
				t_reset();
				prior_fg = -1;
				prior_bg = -1;
			}
			if (cell->fg_rgb >= 0 && cell->fg_rgb != prior_fg) {
				prior_fg = cell->fg_rgb;
				t_foreground_256(cell->fg_rgb);
			}
			if (cell->bg_rgb >= 0 && cell->bg_rgb != prior_bg) {
				prior_bg = cell->bg_rgb;
				t_background_256(cell->bg_rgb);
			}

			/* write out */
			enum t_status stat;
			if ((stat = t_write(&cell->ch, 1)) < 0) {
				return stat;
			}
		}
	}
	return T_OK;
}
