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

static inline int32_t
t__coordinate_system_transform(
	struct t_coordinate_system *system,
	int32_t container_size,
	int32_t object_size,
	int32_t c
) {
	return 
		(system->gravity   * container_size) / 2 +
		(system->alignment * object_size) / 2 +
		(system->direction * c) +
		(system->direction * system->origin);
}

static inline struct t_box *
t__coordinate_system_transform_box(
	struct t_coordinate_system *xsys,
	struct t_coordinate_system *ysys,
	struct t_box *dst,
	struct t_box *src,
	struct t_box *container
) {
	dst->x0 = t__coordinate_system_transform(
		xsys, T_BOX_WIDTH(container), 0, src->x0);
	dst->x1 = t__coordinate_system_transform(
		xsys, T_BOX_WIDTH(container), 0, src->x1);

	dst->y0 = t__coordinate_system_transform(
		ysys, T_BOX_HEIGHT(container), 0, src->y0);
	dst->y1 = t__coordinate_system_transform(
		ysys, T_BOX_HEIGHT(container), 0, src->y1);

	return t_box_standardize(dst, dst);
}

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
	t_frame_context_reset_everything(frame);
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
t_frame_context_set_gravity(
	struct t_frame *dst,
	enum t_gravity xgrav,
	enum t_gravity ygrav
) {
	if (!dst) return T_ENULL;

	dst->context.x.gravity = xgrav;
	dst->context.y.gravity = ygrav;

	return T_OK;
}

enum t_status
t_frame_context_set_alignment(
	struct t_frame *dst,
	enum t_alignment xalign,
	enum t_alignment yalign
) {
	if (!dst) return T_ENULL;

	dst->context.x.alignment = xalign;
	dst->context.y.alignment = yalign;

	return T_OK;
}

enum t_status
t_frame_context_set_direction(
	struct t_frame *dst,
	enum t_direction xdir,
	enum t_direction ydir
) {
	if (!dst) return T_ENULL;

	dst->context.x.direction = xdir;
	dst->context.y.direction = ydir;

	return T_OK;
}

enum t_status
t_frame_context_set_origin(
	struct t_frame *dst,
	int32_t x,
	int32_t y
) {
	if (!dst) return T_ENULL;

	dst->context.x.origin = x;
	dst->context.y.origin = y;

	return T_OK;
}

enum t_status
t_frame_context_reset_clip(
	struct t_frame *dst
) {
	if (!dst) return T_ENULL;

	dst->context.clip.x0 = -T__COORD_INF;
	dst->context.clip.y0 = -T__COORD_INF;
	dst->context.clip.x1 =  T__COORD_INF;
	dst->context.clip.y1 =  T__COORD_INF;

	return T_OK;
}

enum t_status
t_frame_context_reset_coordinate_system(
	struct t_frame *dst
) {
	if (!dst) return T_ENULL;

	dst->context.x.gravity   = T_GRAVITY_LEFT;
	dst->context.x.alignment = T_ALIGNMENT_LEFT;
	dst->context.x.direction = T_DIRECTION_L2R;
	dst->context.x.origin    = 0;

	dst->context.y.gravity   = T_GRAVITY_TOP;
	dst->context.y.alignment = T_ALIGNMENT_TOP;
	dst->context.y.direction = T_DIRECTION_T2B;
	dst->context.y.origin    = 0;

	return T_OK;
}

enum t_status
t_frame_context_reset_everything(
	struct t_frame *dst
) {
	if (!dst) return T_ENULL;

	t_frame_context_reset_clip(dst);
	t_frame_context_reset_coordinate_system(dst);

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
	uint32_t fg_rgba,
	uint32_t bg_rgba
) {
	if (!dst) return T_ENULL;

	for (uint32_t y = 0; y < dst->height; ++y) {
		for (uint32_t x = 0; x < dst->width; ++x) {
			struct t_cell *cell = t__frame_cell_at(dst, x, y);
			cell->fg_rgba = fg_rgba;
			cell->bg_rgba = bg_rgba;
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

	struct t_box bb = T_BOX_SCREEN(dst->width, dst->height);
	t_box_intersect(&bb, &bb, &dst->context.clip);

	for (int32_t y = bb.y0; y < bb.y1; ++y) {
		for (int32_t x = bb.x0; x < bb.x1; ++x) {
			struct t_cell *cell = t__frame_cell_at(dst, x, y);
			if (cell->ch == from) {
				if (flags & T_MAP_CH) cell->ch = to;
				if (flags & T_MAP_ALTFG) cell->fg_rgba = alt_fg_rgba;
				if (flags & T_MAP_ALTBG) cell->bg_rgba = alt_bg_rgba;
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
	uint32_t flat_fg_rgba,
	uint32_t flat_bg_rgba,
	int32_t x,
	int32_t y
) {
	if (!dst || !src) return T_ENULL;

	struct t_box bb = T_BOX_SCREEN(dst->width, dst->height);
	t_box_intersect(&bb, &bb, &dst->context.clip);
	t_box_intersect(&bb, &bb, &T_BOX_GEOM(x, y, src->width, src->height));

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

	for (int32_t bb_y = bb.y0; bb_y < bb.y1; ++bb_y) {
		for (int32_t bb_x = bb.x0; bb_x < bb.x1; ++bb_x) {
			
			struct t_cell *src_cell = t__frame_cell_at(src, bb_x - x, bb_y - y);
			struct t_cell *dst_cell = t__frame_cell_at(dst, bb_x, bb_y);
			if (!src_cell->ch) {
				continue;
			}
			
			dst_cell->ch = (mask & T_BLEND_CH) ?
				src_cell->ch : dst_cell->ch;

			uint32_t const src_fg_rgba = flags & T_BLEND_ALTFG ?
				flat_fg_rgba : src_cell->fg_rgba;
			uint32_t const src_bg_rgba = flags & T_BLEND_ALTBG ?
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
	struct t_frame *src,
	int32_t x,
	int32_t y
) {
	if (!src) return T_ENULL;

	int32_t term_w, term_h;
	t_termsize(&term_w, &term_h);

	/* coordinate transform into canonical upper-left origin, positive 
	 * x right, positive y down system */
	x = t__coordinate_system_transform(
		&src->context.x, term_w, src->width, x);
	y = t__coordinate_system_transform(
		&src->context.y, term_h, src->height, y);

	struct t_box dst_bb = T_BOX_SCREEN(term_w, term_h);
	struct t_box src_bb = T_BOX_GEOM(x, y, src->width, src->height);

	struct t_box clip_bb;
	t__coordinate_system_transform_box(&src->context.x, &src->context.y,
		&clip_bb, &src->context.clip, &dst_bb);

	struct t_box term_bb;
	t_box_intersect(&src_bb, &src_bb, &clip_bb);
	t_box_intersect(&term_bb, &dst_bb, &src_bb);

	t_box_translate(&src_bb, &term_bb, -x, -y);

	int32_t const true_w = T_BOX_WIDTH(&term_bb);
	int32_t const true_h = T_BOX_HEIGHT(&term_bb);

	/* any constants less than -1 required for init */
	int32_t prior_x = -T__COORD_INF;
	int32_t prior_y = -T__COORD_INF;

	int32_t prior_fg = T_RGBA(0, 0, 0, 0);
	int32_t prior_bg = T_RGBA(0, 0, 0, 0);
	t_reset();

	for (int32_t j = 0; j < true_h; ++j) {
		for (int32_t i = 0; i < true_w; ++i) {
			int32_t const
				src_x = src_bb.x0 + i,
				src_y = src_bb.y0 + j;

			// struct t_cell *cell = t__frame_cell_at(src, bb_x - x, bb_y - y);
			struct t_cell *cell = t__frame_cell_at(src, src_x, src_y);
			if (!cell->ch) {
				continue;
			}

			int32_t const
				term_x = term_bb.x0 + i,
				term_y = term_bb.y0 + j;

			/* CURSOR POS */
			int32_t delta_x = term_x - prior_x;
			int32_t delta_y = term_y - prior_y;

			/* @SPEED(max): take a look into this a bit more */
			/* @NOTE(max): converting delta_y into \v chars does NOT
			 * improve throughput significantly.
			 */
			/* minimaly optimize byte usage for relocation */
			if (delta_x && delta_y) {
				t_cursor_pos(term_x + 1, term_y + 1);
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
			prior_x = term_x + 1; /* to account for cursor advancing right when writing */
			prior_y = term_y;
			
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

			/* WRITE OUT */
			enum t_status stat;
			if ((stat = t_write(&cell->ch, 1)) < 0) {
				return stat;
			}
		}
	}
	return T_OK;
}
