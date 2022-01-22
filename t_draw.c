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
	return &frame->grid[(y * frame->width) + x];
}

enum t_status
t_frame_create(
	struct t_frame *out,
	uint32_t width,
	uint32_t height
) {
	if (!out || !width || !height) return T_ENULL;

	out->grid = calloc(width * height, sizeof(struct t_frame));
	out->width = width;
	out->height = height;

	return T_OK;
}

enum t_status
t_frame_create_pattern(
	struct t_frame *out,
	char const *pattern,
	enum t_frame_flag flags
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
t_frame_clear(
	struct t_frame *frame
) {
	if (!frame) return T_ENULL;

	memset(frame->grid, 0, sizeof(struct t_cell) * (frame->width * frame->height));

	return T_OK;
}

enum t_status
t_frame_overlay(
	struct t_frame *dst,
	struct t_frame *src,
	int32_t x,
	int32_t y
) {
	return T_OK;
}

enum t_status
t_frame_paint(
	struct t_frame *dst,
	int fg_rgb,
	int bg_rgb
) {
	if (!dst) return T_ENULL;

	uint8_t fg_r = T_RED(fg_rgb),
			fg_g = T_GREEN(fg_rgb),
			fg_b = T_BLUE(fg_rgb);

	uint8_t bg_r = T_RED(bg_rgb),
			bg_g = T_GREEN(bg_rgb),
			bg_b = T_BLUE(bg_rgb);

	for (uint32_t i = 0; i < (dst->width * dst->height); ++i) {
		struct t_cell *cell = &dst->grid[i];
		if (fg_rgb >= 0) {
			cell->fg.r = fg_r;
			cell->fg.g = fg_g;
			cell->fg.b = fg_b;
		}
		if (bg_rgb >= 0) {
			cell->bg.r = bg_r;
			cell->bg.g = bg_g;
			cell->bg.b = bg_b;
		}
	}
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

	uint32_t span_x = bb_x1 >= bb_x0 ? bb_x1 - bb_x0 : 0;
	uint32_t span_y = bb_y1 >= bb_y0 ? bb_y1 - bb_y0 : 0;

	/* any constants less than -1 required for init */
	int32_t prior_x = -65535;
	int32_t prior_y = -65535;

	uint32_t prior_fg = -1;
	uint32_t prior_bg = -1;

	/* use single for-loop to avoid edge cases when span_x = 0 or span_y = 0 */
	for (uint32_t i = 0; i < (span_x * span_y); ++i) {
		int32_t bb_x = (i % span_x) + bb_x0;
		int32_t bb_y = (i / span_x) + bb_y0;

		struct t_cell *cell = t__frame_cell_at(frame, bb_x - x, bb_y - y);
		if (!cell->ch) {
			continue;
		}

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
		
		uint32_t now_fg = T_RGB(
			cell->fg.r,
			cell->fg.g,
			cell->fg.b
		);
		uint32_t now_bg = T_RGB(
			cell->bg.r,
			cell->bg.g,
			cell->bg.b
		);

		/* update prior */
		if (now_fg != prior_fg) {
			prior_fg = now_fg;
			t_foreground_256(now_fg);
		}
		if (now_bg != prior_bg) {
			prior_bg = now_bg;
			t_background_256(now_bg);
		}
		prior_x = bb_x;
		prior_y = bb_y;

		/* write out */
		enum t_status stat;
		if ((stat = t_write(&cell->ch, 1)) < 0) {
			return stat;
		}
	}
	return T_OK;
}
