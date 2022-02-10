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
static inline void
t__frame_zero(
	struct t_frame *frame
) {
	memset(frame, 0, sizeof(struct t_frame));
}

static inline void
t__pattern_dimensions(
	char const *pattern,
	int32_t *out_width,
	int32_t *out_height
) {
	int32_t width = 0;
	int32_t height = 0;
	int32_t row_width = 0;

	while (*pattern) {
		switch (*pattern) {
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
		++pattern;
	}

	if (out_width) *out_width = width;
	if (out_height) *out_height = height;
}

static inline void
t__pattern_impress(
	struct t_frame *dst,
	char const *pattern
) {
	int32_t x = 0;
	int32_t y = 0;

	while (*pattern) {
		switch (*pattern) {
		case '\n':
			++y;
		case '\r':
			x = 0;
			break;
		case '\v':
			++y;
			break;
		default:
			t_frame_cell_at(dst, x, y)->ch = *pattern;
			++x;
			break;
		}
		++pattern;
	}
}

/* +--- FRAME MANAGING --------------------------------------------+ */
enum t_status
t_frame_create(
	struct t_frame *dst,
	int32_t width,
	int32_t height
) {
	t__frame_zero(dst);
	return t_frame_resize(dst, width, height);
}

enum t_status
t_frame_create_pattern(
	struct t_frame *dst,
	char const *pattern
) {
	int32_t width = 0;
	int32_t height = 0;
	t__pattern_dimensions(pattern, &width, &height);

	enum t_status create_stat;
	if ((create_stat = t_frame_create(dst, width, height)) < 0) {
		return create_stat;
	}

	t__pattern_impress(dst, pattern);
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

/* +--- MANUAL ----------------------------------------------------+ */
enum t_status
t_frame_init_pattern(
	struct t_frame *dst,
	char const *pattern
) {
	int32_t width;
	int32_t height;
	t__pattern_dimensions(pattern, &width, &height);

	if ((dst->width && dst->width != width) ||
		(dst->height && dst->height != height))
	{
		return T_EMISMATCH;
	}

	dst->width = width;
	dst->height = height;

	t__pattern_impress(dst, pattern);
	return T_OK;
}

void
t_frame_compute_clip(
	struct t_box *dst,
	struct t_frame *src
) {
	struct t_box resulting_clip = T_BOX_WH(src->width, src->height);
	if (src->_clip_index) {
		struct t_box *top_clip = &src->_clip_stack[src->_clip_index-1];
		t_box_intersect(
			&resulting_clip,
			&resulting_clip,
			t_box_clamp_offset_to_origin(
				top_clip,
				top_clip
			)
		);
	}
	*dst = resulting_clip;
}

enum t_status
t_frame_push_inset_clip(
	struct t_frame *dst,
	int32_t lx,
	int32_t ly,
	int32_t rx,
	int32_t ry
) {
	if (dst->_clip_index < TC_FRAME_CONTEXT_SLOTS) {
		struct t_box current_clip;
		t_frame_compute_clip(&current_clip, dst);

		struct t_box *dst_clip = &dst->_clip_stack[dst->_clip_index++];
		dst_clip->x0 = current_clip.x0 + lx;
		dst_clip->y0 = current_clip.y0 + ly;
		dst_clip->x1 = current_clip.x1 - lx;
		dst_clip->y1 = current_clip.y1 - ly;

		t_box_clamp_offset_to_origin(dst_clip, dst_clip);
	}
	return T_EOVERFLO;
}

/* +--- DRAWING ---------------------------------------------------+ */
void
t_frame_map(
	struct t_frame *dst,
	uint32_t mode,
	uint32_t rgba_fg,
	uint32_t rgba_bg,
	char from,
	char to
) {
	struct t_box clip;
	t_frame_compute_clip(&clip, dst);

	for (int32_t y = clip.y0; y < clip.y1; ++y) {
		for (int32_t x = clip.x0; x < clip.x1; ++x) {
			struct t_cell *cell = t_frame_cell_at(dst, x, y);
			if (cell->ch == from) {
				if (mode & T_MAP_CH) cell->ch = to;
				if (mode & T_MAP_FOREGROUND) cell->rgba_fg = rgba_fg;
				if (mode & T_MAP_BACKGROUND) cell->rgba_bg = rgba_bg;
			}
		}
	}
}

void
t_frame_clear(
	struct t_frame *frame
) {
	/* @TODO(max): abide by clip */
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
	struct t_box clip;
	t_frame_compute_clip(&clip, dst);

	for (uint32_t y = clip.y0; y < clip.y1; ++y) {
		for (uint32_t x = clip.x0; x < clip.x1; ++x) {
			struct t_cell *cell = t_frame_cell_at(dst, x, y);
			cell->rgba_fg = fg_rgba;
			cell->rgba_bg = bg_rgba;
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
	struct t_box dst_clip, src_clip;
	t_frame_compute_clip(&dst_clip, dst);
	t_frame_compute_clip(&src_clip, src);

	t_box_translate(&src_clip, &src_clip, x, y);
	t_box_intersect(&dst_clip, &dst_clip, &src_clip);
	t_box_translate(&src_clip, &dst_clip, -x, -y);

	for (int32_t j = 0; j < T_BOX_HEIGHT(&dst_clip); ++j) {
		for (int32_t i = 0; i < T_BOX_WIDTH(&dst_clip); ++i) {
			int32_t const
				src_x = src_clip.x0 + i,
				src_y = src_clip.y0 + j;

			int32_t const
				dst_x = dst_clip.x0 + i,
				dst_y = dst_clip.y0 + j;

			struct t_cell *dst_cell = t_frame_cell_at(dst, dst_x, dst_y);
			struct t_cell *src_cell = t_frame_cell_at(src, src_x, src_y);
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
	int32_t term_width, term_height;
	t_termsize(&term_width, &term_height);

	struct t_box dst_clip = T_BOX_WH(term_width, term_height);
	struct t_box src_clip;
	t_frame_compute_clip(&src_clip, src);

	t_box_translate(&src_clip, &src_clip, x, y);
	t_box_intersect(&dst_clip, &dst_clip, &src_clip);
	t_box_translate(&src_clip, &dst_clip, -x, -y);

	/* any constants less than -1 required for init */
	int32_t prior_x = INT32_MIN;
	int32_t prior_y = INT32_MIN;

	int32_t prior_rgba_fg = T_RGBA(0, 0, 0, 0);
	int32_t prior_rgba_bg = T_RGBA(0, 0, 0, 0);
	t_reset();

	for (int32_t j = 0; j < T_BOX_HEIGHT(&dst_clip); ++j) {
		for (int32_t i = 0; i < T_BOX_WIDTH(&dst_clip); ++i) {
			int32_t const
				src_x = src_clip.x0 + i,
				src_y = src_clip.y0 + j;

			struct t_cell *cell = t_frame_cell_at(src, src_x, src_y);
			if (!cell->ch) {
				continue;
			}

			int32_t const
				dst_x = dst_clip.x0 + i,
				dst_y = dst_clip.y0 + j;

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
