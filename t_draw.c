#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "t_base.h"
#include "t_sequence.h"

#include "t_draw.h"


/* +--- COLOR UTILITIES -------------------------------------------+ */
static inline enum t_status
t__foreground_256(
	uint32_t rgba
) {
	return t_foreground_256(t_rgb_compress_256(rgba));
}

static inline enum t_status
t__background_256(
	uint32_t rgba
) {
	return t_background_256(t_rgb_compress_256(rgba));
}

/* +--- FRAME DRAWING ---------------------------------------------+ */
#define T__COORD_INF 65535

struct t__intersection
{
	int32_t width;
	int32_t height;

	struct { int32_t x, y; } dst;
	struct { int32_t x, y; } src;
};

static inline void
t__intersection_compute(
	struct t__intersection *out,
	int32_t dst_w,
	int32_t dst_h,
	int32_t src_w,
	int32_t src_h,
	int32_t src_x,
	int32_t src_y
) {
	int32_t const
		x0 = T_MAX(0, src_x),
		y0 = T_MAX(0, src_y),
		x1 = T_MAX(x0, T_MIN(dst_w, src_x + src_w)),
		y1 = T_MAX(y0, T_MIN(dst_h, src_y + src_h));

	out->width  = x1 - x0;
	out->height = y1 - y0;

	out->dst.x  = x0;
	out->dst.y  = y0;

	out->src.x  = x0 - src_x;
	out->src.y  = y0 - src_y;
}

static inline void
t__frame_zero(
	struct t_frame *frame
) {
	memset(frame, 0, sizeof(struct t_frame));
}

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
	int32_t width,
	int32_t height
) {
	if (!out) return T_ENULL;
	t__frame_zero(out);

	return t_frame_resize(out, width, height);
}

enum t_status
t_frame_create_pattern(
	struct t_frame *out,
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
			t__frame_cell_at(out, x, y)->ch = *iter;
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
	t__frame_zero(frame);
}

enum t_status
t_frame_resize(
	struct t_frame *dst,
	int32_t n_width,
	int32_t n_height
) {
	if (n_width < 0 || n_height < 0) return T_EPARAM;

	uint32_t const old_size = T_ALIGN_UP(
		sizeof(struct t_cell) * dst->width * dst->height, TC_GRID_BLOCK
	);
	uint32_t const new_size = T_ALIGN_UP(
		sizeof(struct t_cell) * n_width * n_height, TC_GRID_BLOCK
	);

	if (new_size <= old_size) {
		dst->width = n_width;
		dst->height = n_height;
		return T_OK;
	}

	struct t_cell *new_grid;
	if (posix_memalign((void *) &new_grid, TC_GRID_ALIGN, new_size)) {
		return T_EMALLOC;
	}

	dst->grid = new_grid;
	dst->width = n_width;
	dst->height = n_height;

	return T_OK;
}

void
t_frame_clear(
	struct t_frame *frame
) {
	memset(frame->grid, 0,
		sizeof(struct t_cell) * 
		frame->width *
		frame->height
	);
}

void
t_frame_paint(
	struct t_frame *dst,
	uint32_t fg_rgba,
	uint32_t bg_rgba
) {
	for (uint32_t y = 0; y < dst->height; ++y) {
		for (uint32_t x = 0; x < dst->width; ++x) {
			struct t_cell *cell = t__frame_cell_at(dst, x, y);
			cell->rgba_fg = fg_rgba;
			cell->rgba_bg = bg_rgba;
		}
	}
}

void
t_frame_map(
	struct t_frame *dst,
	uint32_t mode,
	uint32_t rgba_fg,
	uint32_t rgba_bg,
	char from,
	char to
) {
	for (int32_t y = 0; y < dst->height; ++y) {
		for (int32_t x = 0; x < dst->width; ++x) {
			struct t_cell *cell = t__frame_cell_at(dst, x, y);
			if (cell->ch == from) {
				if (mode & T_MAP_CH) cell->ch = to;
				if (mode & T_MAP_FOREGROUND) cell->rgba_fg = rgba_fg;
				if (mode & T_MAP_BACKGROUND) cell->rgba_bg = rgba_bg;
			}
		}
	}
}

void
t_frame_overlay(
	struct t_frame *dst,
	struct t_frame *src,
	int32_t x,
	int32_t y
) {
	struct t__intersection box;
	t__intersection_compute(&box,
		dst->width, dst->height,
		src->width, src->height,
		x, y
	);

	for (int32_t j = 0; j < box.height; ++j) {
		for (int32_t i = 0; i < box.width; ++i) {
			int32_t const
				src_x = box.src.x + i,
				src_y = box.src.y + j;

			int32_t const
				dst_x = box.dst.x + i,
				dst_y = box.dst.y + j;

			struct t_cell *dst_cell = t__frame_cell_at(dst, dst_x, dst_y);
			struct t_cell *src_cell = t__frame_cell_at(src, src_x, src_y);
			if (!src_cell->ch) {
				continue;
			}

			dst_cell->ch      = src_cell->ch;
			dst_cell->rgba_fg = src_cell->rgba_fg;
			dst_cell->rgba_bg = src_cell->rgba_bg;
		}
	}
}

enum t_status
t_frame_rasterize(
	struct t_frame *src,
	int32_t x,
	int32_t y
) {
	if (!src) return T_ENULL;

	int32_t term_w, term_h;
	t_termsize(&term_w, &term_h);

	struct t__intersection box;
	t__intersection_compute(&box,
		term_w, term_h,
		src->width, src->height,
		x, y
	);

	/* any constants less than -1 required for init */
	int32_t prior_x = -T__COORD_INF;
	int32_t prior_y = -T__COORD_INF;

	int32_t prior_rgba_fg = T_RGBA(0, 0, 0, 0);
	int32_t prior_rgba_bg = T_RGBA(0, 0, 0, 0);
	t_reset();

	for (int32_t j = 0; j < box.height; ++j) {
		for (int32_t i = 0; i < box.width; ++i) {
			int32_t const
				src_x = box.src.x + i,
				src_y = box.src.y + j;

			struct t_cell *cell = t__frame_cell_at(src, src_x, src_y);
			if (!cell->ch) {
				continue;
			}

			int32_t const
				dst_x = box.dst.x + i,
				dst_y = box.dst.y + j;

			/* CURSOR POS */
			int32_t delta_x = dst_x - prior_x;
			int32_t delta_y = dst_y - prior_y;

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
			uint8_t const fg_alpha = T_ALPHA(cell->rgba_fg);
			uint8_t const bg_alpha = T_ALPHA(cell->rgba_bg);

			if (cell->rgba_fg != prior_rgba_fg || 
				cell->rgba_bg != prior_rgba_bg) 
			{
				if (!fg_alpha || !bg_alpha) {
					t_reset();
					prior_rgba_fg = T_WASHED;
					prior_rgba_bg = T_WASHED;
				}
				if (cell->rgba_fg != prior_rgba_fg) {
					prior_rgba_fg = cell->rgba_fg;
					t__foreground_256(cell->rgba_fg);
				}
				if (cell->rgba_bg != prior_rgba_bg) {
					prior_rgba_bg = cell->rgba_bg;
					t__background_256(cell->rgba_bg);
				}
			}

			/* WRITE OUT */
			enum t_status stat;
			if ((stat = t_write(&cell->ch, 1)) < 0) {
				return stat;
			}
		}
	}
	return T_OK;
}
