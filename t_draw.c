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
static inline void
t__frame_zero(
	struct t_frame *frame
) {
	memset(frame, 0, sizeof(struct t_frame));
}

static inline void
t__frame_reset(
	struct t_frame *frame
) {
	t_frame_blend_reset(frame);
}

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
	int32_t width,
	int32_t height
) {
	if (!out) return T_ENULL;
	t__frame_zero(out);
	t__frame_reset(out);

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
	t__frame_reset(out);

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
	t__frame_zero(frame);
}

enum t_status
t_frame_resize(
	struct t_frame *dst,
	int32_t n_width,
	int32_t n_height
) {
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
	int32_t fg_rgba,
	int32_t bg_rgba
) {
	if (!dst) return T_ENULL;

	for (uint32_t j = 0; j < dst->height; ++j) {
		for (uint32_t i = 0; i < dst->width; ++i) {
			struct t_cell *cell = t__frame_cell_at(dst, i, j);
			cell->fg_rgba = fg_rgba;
			cell->bg_rgba = bg_rgba;
		}
	}
	return T_OK;
}

enum t_status
t_frame_blend_reset(
	struct t_frame *dst
) {
	if (!dst) return T_ENULL;

	dst->blend.clip.x0 = INT32_MIN;
	dst->blend.clip.y0 = INT32_MIN;
	dst->blend.clip.x1 = INT32_MAX;
	dst->blend.clip.y1 = INT32_MAX;

	return T_OK;
}

enum t_status
t_frame_blend(
	struct t_frame *dst,
	struct t_frame *src,
	enum t_blend_flag flags,
	int32_t flat_fg_rgba,
	int32_t flat_bg_rgba,
	int32_t x,
	int32_t y
) {
	if (!dst || !src) return T_ENULL;

	struct t_box bb = T_BOX_SCREEN(dst->width, dst->height);
	t_box_intersect_no_standardize(
		&bb, t_box_standardize(&dst->blend.clip)
	);
	t_box_intersect_no_standardize(
		&bb, &T_BOX_GEOM(x, y, src->width, src->height)
	);

	/*
	int32_t
		bb_x0 = T_MAX(0, x),
		bb_y0 = T_MAX(0, y),
		bb_x1 = T_MIN(dst->width,  x + src->width),
		bb_y1 = T_MIN(dst->height, y + src->height);
	*/

	uint32_t const dst_rgba_mask = 
		(flags & T_BLEND_R ? 0 : T_MASK_R) |
		(flags & T_BLEND_G ? 0 : T_MASK_G) |
		(flags & T_BLEND_B ? 0 : T_MASK_B) |
		(flags & T_BLEND_A ? 0 : T_MASK_A);

	uint32_t const src_rgba_mask =
		(flags & T_BLEND_R ? T_MASK_R : 0) |
		(flags & T_BLEND_G ? T_MASK_G : 0) |
		(flags & T_BLEND_B ? T_MASK_B : 0) |
		(flags & T_BLEND_A ? T_MASK_A : 0);

	for (int32_t bb_y = bb.y0; bb_y < bb.y1; ++bb_y) {
		for (int32_t bb_x = bb.x0; bb_x < bb.x1; ++bb_x) {
			
			struct t_cell *src_cell = t__frame_cell_at(src, bb_x - x, bb_y - y);
			struct t_cell *dst_cell = t__frame_cell_at(dst, bb_x, bb_y);
			if (!src_cell->ch) {
				continue;
			}
			
			dst_cell->ch = (flags & T_BLEND_CH) ?
				src_cell->ch : dst_cell->ch;

			uint32_t const src_fg_rgba = flags & T_BLEND_FGOVERRIDE ?
				flat_fg_rgba : src_cell->fg_rgba;
			uint32_t const src_bg_rgba = flags & T_BLEND_BGOVERRIDE ?
				flat_bg_rgba : src_cell->bg_rgba;

			uint32_t const dst_fg_rgba = dst_cell->fg_rgba;
			uint32_t const dst_bg_rgba = dst_cell->bg_rgba;

			dst_cell->fg_rgba = (src_fg_rgba & src_rgba_mask) |
			                    (dst_fg_rgba & dst_rgba_mask);
			dst_cell->bg_rgba = (src_bg_rgba & src_rgba_mask) |
			                    (dst_bg_rgba & dst_rgba_mask);
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

	/* any constants less than -1 required for init */
	int32_t prior_x = -65535;
	int32_t prior_y = -65535;

	int32_t prior_fg = T_RGBA(0, 0, 0, 0);
	int32_t prior_bg = T_RGBA(0, 0, 0, 0);
	t_reset();

	for (int32_t bb_y = bb_y0; bb_y < bb_y1; ++bb_y) {
		for (int32_t bb_x = bb_x0; bb_x < bb_x1; ++bb_x) {

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
			uint8_t const fg_alpha = T_ALPHA(cell->fg_rgba);
			uint8_t const bg_alpha = T_ALPHA(cell->bg_rgba);

			if (cell->fg_rgba != prior_fg || cell->bg_rgba != prior_bg) {
				if (!fg_alpha || !bg_alpha) {
					t_reset();
					prior_fg = T_WASHED;
					prior_bg = T_WASHED;
				}
				if (cell->fg_rgba != prior_fg) {
					prior_fg = cell->fg_rgba;
					t__foreground_256(cell->fg_rgba);
				}
				if (cell->bg_rgba != prior_bg) {
					prior_bg = cell->bg_rgba;
					t__background_256(cell->bg_rgba);
				}
			}
#if 0
			if (!fg_alpha || !bg_alpha) {
				t_reset();
				prior_fg = T_WASHED;
				prior_bg = T_WASHED;
			}
			if (fg_alpha > 0 && cell->fg_rgba != prior_fg) {
				prior_fg = cell->fg_rgba;
				t__foreground_256(cell->fg_rgba);
			}
			if (bg_alpha > 0 && cell->bg_rgba != prior_bg) {
				prior_bg = cell->bg_rgba;
				t__background_256(cell->bg_rgba);
			}
#endif

			/* WRITE OUT */
			enum t_status stat;
			if ((stat = t_write(&cell->ch, 1)) < 0) {
				return stat;
			}
		}
	}
	return T_OK;
}
