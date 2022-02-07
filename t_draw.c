#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "t_base.h"
#include "t_sequence.h"

#include "t_draw.h"


/* +--- COLOR UTILITIES -------------------------------------------+ */
uint8_t
t_rgb_compress_cube_256(
	uint32_t rgb
) {
	static int const l_point_table [] = {
		0x00, 0x5f, 0x87, 0xaf, 0xd7, 0xff,
	};
	int const components [3] = {
		T_RED(rgb), T_GREEN(rgb), T_BLUE(rgb),
	};
	int cube [3];

	/* @SPEED(max): check compiler output for unrolling to make sure */
	for (int i = 0; i < 3; ++i) {
		int close_diff = 255;
		for (int j = 0; j < 6; ++j) {
			int diff = T_ABS(l_point_table[j] - components[i]);
			if (diff < close_diff) {
				cube[i] = j;
				close_diff = diff;
			}
		}
	}

	return 16 +
	       36 * cube[0] +
	        6 * cube[1] +
	            cube[2];
}

uint8_t
t_rgb_compress_gray_256(
	uint32_t rgb
) {
	int const scale = (T_RED(rgb) + T_GREEN(rgb) + T_BLUE(rgb)) / 3;
	int const index = (scale - 0x08) / 10;

	/* because grayscale doesn't end with 0xffffff, we have to do this
	 * stupid magic to wrap the 256 to a 231 which happens to be the
	 * another code for 0xffffff */
	return 232 + (index % 24) - (index / 24);
}

uint8_t
t_rgb_compress_256(
	uint32_t rgb
) {
	/* @SPEED(max): branch */
	if (T_RED(rgb) == T_GREEN(rgb) && T_GREEN(rgb) == T_BLUE(rgb)) {
		return t_rgb_compress_gray_256(rgb);
	}
	return t_rgb_compress_cube_256(rgb);
}

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
	enum t_frame_flag flags,
	char const *pattern
) {
	if (!out) return T_ENULL;
	t__frame_zero(out);

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
				t__frame_cell_at(out, x, y)->letter = *iter;
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
	t__frame_zero(frame);
}

enum t_status
t_frame_resize(
	struct t_frame *dst,
	int32_t n_width,
	int32_t n_height
) {
#if 0
	if (!dst) return T_ENULL;
	if (n_width < 0 || n_height < 0) return T_EPARAM;
	if (!n_width || !n_height) return T_OK;

	int32_t const n_true_width = T_ALIGN_UP(n_width, TC_CELL_BLOCK_WIDTH);
	int32_t const n_true_height = T_ALIGN_UP(n_height, TC_CELL_BLOCK_HEIGHT);

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
#else
	if (!dst) return T_ENULL;
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
#endif
}

enum t_status
t_frame_clear(
	struct t_frame *frame
) {
	if (!frame) return T_ENULL;

	memset(frame->grid, 0,
		sizeof(struct t_cell) * 
		frame->width *
		frame->height
	);

	return T_OK;
}

enum t_status
t_frame_paint(
	struct t_frame *dst,
	uint32_t fg_rgba,
	uint32_t bg_rgba
) {
	if (!dst) return T_ENULL;

	for (uint32_t y = 0; y < dst->height; ++y) {
		for (uint32_t x = 0; x < dst->width; ++x) {
			struct t_cell *cell = t__frame_cell_at(dst, x, y);
			cell->foreground = fg_rgba;
			cell->background = bg_rgba;
		}
	}
	return T_OK;
}

enum t_status
t_frame_map_one(
	struct t_frame *dst,
	enum t_map_flag flags,
	uint32_t alt_fg_rgba,
	uint32_t alt_bg_rgba,
	char from,
	char to
) {
	if (!dst) return T_ENULL;

	for (int32_t y = 0; y < dst->height; ++y) {
		for (int32_t x = 0; x < dst->width; ++x) {
			struct t_cell *cell = t__frame_cell_at(dst, x, y);
			if (cell->letter == from) {
				if (flags & T_MAP_CH) cell->letter = to;
				if (flags & T_MAP_ALTFG) cell->foreground = alt_fg_rgba;
				if (flags & T_MAP_ALTBG) cell->background = alt_bg_rgba;
			}
		}
	}
	return T_OK;
}

enum t_status
t_frame_blend(
	struct t_frame *dst,
	struct t_frame *src,
	enum t_blend_mask mask,
	enum t_blend_flag flags,
	uint32_t alt_fg_rgba,
	uint32_t alt_bg_rgba,
	int32_t x,
	int32_t y
) {
	if (!dst || !src) return T_ENULL;

	struct t__intersection box;
	t__intersection_compute(&box,
		dst->width, dst->height,
		src->width, src->height,
		x, y
	);

	uint32_t const dst_rgba_mask = 
		(mask & T_BLEND_R ? 0 : T_MASK_R) |
		(mask & T_BLEND_G ? 0 : T_MASK_G) |
		(mask & T_BLEND_B ? 0 : T_MASK_B) |
		(mask & T_BLEND_A ? 0 : T_MASK_A);

	uint32_t const src_rgba_mask =
		(mask & T_BLEND_R ? T_MASK_R : 0) |
		(mask & T_BLEND_G ? T_MASK_G : 0) |
		(mask & T_BLEND_B ? T_MASK_B : 0) |
		(mask & T_BLEND_A ? T_MASK_A : 0);

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
			if (!src_cell->letter) {
				continue;
			}
			
			dst_cell->letter = (mask & T_BLEND_CH) ?
				src_cell->letter : dst_cell->letter;

			uint32_t const src_fg_rgba = flags & T_BLEND_ALTFG ?
				alt_fg_rgba : src_cell->foreground;
			uint32_t const src_bg_rgba = flags & T_BLEND_ALTBG ?
				alt_bg_rgba : src_cell->background;

			uint32_t const dst_fg_rgba = dst_cell->foreground;
			uint32_t const dst_bg_rgba = dst_cell->background;

			dst_cell->foreground = (src_fg_rgba & src_rgba_mask) |
			                       (dst_fg_rgba & dst_rgba_mask);
			dst_cell->background = (src_bg_rgba & src_rgba_mask) |
			                       (dst_bg_rgba & dst_rgba_mask);
		}
	}
	return T_OK;
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

	int32_t prior_foreground = T_RGBA(0, 0, 0, 0);
	int32_t prior_background = T_RGBA(0, 0, 0, 0);
	t_reset();

	for (int32_t j = 0; j < box.height; ++j) {
		for (int32_t i = 0; i < box.width; ++i) {
			int32_t const
				src_x = box.src.x + i,
				src_y = box.src.y + j;

			struct t_cell *cell = t__frame_cell_at(src, src_x, src_y);
			if (!cell->letter) {
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
			uint8_t const fg_alpha = T_ALPHA(cell->foreground);
			uint8_t const bg_alpha = T_ALPHA(cell->background);

			if (cell->foreground != prior_foreground || 
				cell->background != prior_background) 
			{
				if (!fg_alpha || !bg_alpha) {
					t_reset();
					prior_foreground = T_WASHED;
					prior_background = T_WASHED;
				}
				if (cell->foreground != prior_foreground) {
					prior_foreground = cell->foreground;
					t__foreground_256(cell->foreground);
				}
				if (cell->background != prior_background) {
					prior_background = cell->background;
					t__background_256(cell->background);
				}
			}

			/* WRITE OUT */
			enum t_status stat;
			if ((stat = t_write(&cell->letter, 1)) < 0) {
				return stat;
			}
		}
	}
	return T_OK;
}
