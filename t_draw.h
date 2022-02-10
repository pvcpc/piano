#ifndef INCLUDE_T_DRAW_H
#define INCLUDE_T_DRAW_H

#include "t_util.h"
#include "t_sequence.h"

/* @TUNABLE compile-time constants (see README):
 * - TC_CELL_BLOCK_WIDTH (default 16):
 *   Set the chunk size by which `t_frame` width will actually be
 *   allocated to minimize malloc(3)/free(3) calls when resizing.
 *
 * - TC_CELL_BLOCK_HEIGHT (default 16):
 *   Set the chunk size by which `t_frame` height will actually be
 *   allocated to minimize malloc(3)/free(3) calls when resizing.
 *
 * - TC_GRID_ALIGN (default 16):
 * - TC_GRID_BLOCK(default 16):
 *
 * - TC_FRAME_CONTEXT_SLOTS (default 4):
 */
#ifndef TC_CELL_BLOCK_WIDTH
#  define TC_CELL_BLOCK_WIDTH 16
#endif

#ifndef TC_CELL_BLOCK_HEIGHT
#  define TC_CELL_BLOCK_HEIGHT 16
#endif

#ifndef TC_GRID_ALIGN
#  define TC_GRID_ALIGN 16
#endif

#ifndef TC_GRID_BLOCK
#  define TC_GRID_BLOCK 16
#endif

#ifndef TC_FRAME_CONTEXT_SLOTS
#  define TC_FRAME_CONTEXT_SLOTS 4
#endif

/* +--- UTILITIES -------------------------------------------------+ */
#define T_RGBA(r, g, b, a) (   \
	(((a) & 0xff) << 24) | \
	(((b) & 0xff) << 16) | \
	(((g) & 0xff) <<  8) | \
	(((r) & 0xff)      )   \
)
#define T_RGB(r, g, b) T_RGBA(r, g, b, 255)
#define T_GRAY(s)      T_RGBA(s, s, s, 255)
#define T_WASHED       T_RGBA(0, 0, 0, 0)

#define T_RED(rgba)   (((rgba)      ) & 0xff)
#define T_GREEN(rgba) (((rgba) >>  8) & 0xff)
#define T_BLUE(rgba)  (((rgba) >> 16) & 0xff)
#define T_ALPHA(rgba) (((rgba) >> 24) & 0xff)

#define T_MASK_R 0x000000ff
#define T_MASK_G 0x0000ff00
#define T_MASK_B 0x00ff0000
#define T_MASK_A 0xff000000

static inline uint8_t
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

static inline uint8_t
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

static inline uint8_t
t_rgb_compress_256(
	uint32_t rgb
) {
	/* @SPEED(max): branch */
	if (T_RED(rgb) == T_GREEN(rgb) && T_GREEN(rgb) == T_BLUE(rgb)) {
		return t_rgb_compress_gray_256(rgb);
	}
	return t_rgb_compress_cube_256(rgb);
}

#define t_foreground_256_rgba(rgba) \
	t_foreground_256(t_rgb_compress_256(rgba))
#define t_background_256_rgba(rgba) \
	t_background_256(t_rgb_compress_256(rgba))

#define t_foreground_256_ex(r, g, b) \
	t_foreground_256_rgba(T_RGB(r, g, b))
#define t_background_256_ex(r, g, b) \
	t_background_256_rgba(T_RGB(r, g, b))

struct t_box
{
	int32_t x0, y0;
	int32_t x1, y1;
};

#define T_BOX(x, y, width, height) \
	((struct t_box){ x, y, width, height, })
#define T_BOX_WH(width, height) \
	((struct t_box){ 0, 0, width, height, })

#define T_BOX_WIDTH(box_ptr) \
	((box_ptr)->x1 - (box_ptr)->x0)
#define T_BOX_HEIGHT(box_ptr) \
	((box_ptr)->y1 - (box_ptr)->y0)

static inline struct t_box *
t_box_translate(
	struct t_box *dst,
	struct t_box const *src,
	int32_t x,
	int32_t y
) {
	dst->x0 = src->x0 + x;
	dst->y0 = src->y0 + y;
	dst->x1 = src->x1 + x;
	dst->y1 = src->y1 + y;
	return dst;
}

static inline struct t_box *
t_box_clamp_offset_to_origin(
	struct t_box *dst,
	struct t_box *src
) {
	/* ensures x1 >= x0 && y1 >= y0 by clamping x1, y1 to be above
	 * x0, y0 if necessary */
	dst->x0 = src->x0;
	dst->y0 = src->y0;
	dst->x1 = T_MAX(src->x0, src->x1);
	dst->y1 = T_MAX(src->y0, src->y1);
	return dst;
}

static inline struct t_box *
t_box_standardize(
	struct t_box *dst,
	struct t_box const *src
) {
	/* @NOTE(max): by standard form we mean x0 <= x1 && y0 <= y1,
	 * so this function swaps values to make the condition true as
	 * opposed to clamping x1 to x0 if x1 < x0 or y1 to y0 if y0 < y1
	 * (see `t_box_clamp_offset_to_origin`).
	 */
	int32_t const
		tx0 = T_MIN(src->x0, src->x1),
		ty0 = T_MIN(src->y0, src->y1),
		tx1 = T_MAX(src->x0, src->x1),
		ty1 = T_MAX(src->y0, src->y1);
	dst->x0 = tx0;
	dst->y0 = ty0;
	dst->x1 = tx1;
	dst->y1 = ty1;
	return dst;
}

static inline struct t_box *
t_box_intersect_boff(
	struct t_box *dst,
	struct t_box const *boxa,
	struct t_box const *boxb,
	int32_t boffx,
	int32_t boffy
) {
	/* @NOTE(max): boxa and boxb assumed to be in standard form */
	int32_t const
		tx0 = T_MAX(boxa->x0, boxb->x0 + boffx),
		ty0 = T_MAX(boxa->y0, boxb->y0 + boffy),
		tx1 = T_MIN(boxa->x1, boxb->x1 + boffx),
		ty1 = T_MIN(boxa->y1, boxb->y1 + boffy);
	dst->x0 = tx0;
	dst->y0 = ty0;
	dst->x1 = tx1;
	dst->y1 = ty1;
	return t_box_clamp_offset_to_origin(dst, dst);
}

static inline struct t_box *
t_box_intersect(
	struct t_box *dst,
	struct t_box const *boxa,
	struct t_box const *boxb
) {
	return t_box_intersect_boff(dst, boxa, boxb, 0, 0);
}

/* +--- FRAME -----------------------------------------------------+ */
struct t_cell
{
	/* see t_sequence.h for how color is packed */
	uint32_t       rgba_fg; 
	uint32_t       rgba_bg;
	uint8_t        ch;
};

struct t_frame
{
	struct t_cell *grid;
	int32_t        width;
	int32_t        height;

	/* context */
	struct t_box   _clip_stack [TC_FRAME_CONTEXT_SLOTS];
	uint32_t       _clip_index;
};


/* +--- AUTOMATIC MANAGEMENT --------------------------------------+ */
enum t_status
t_frame_create(
	struct t_frame *dst,
	int32_t width,
	int32_t height
);

enum t_status
t_frame_create_pattern(
	struct t_frame *dst,
	char const *pattern
);

void
t_frame_destroy(
	struct t_frame *dst
);

enum t_status
t_frame_resize(
	struct t_frame *dst,
	int32_t n_width,
	int32_t n_height
);

/* +--- MANUAL MANAGEMENT -----------------------------------------+ */
#define T_SCRATCH_GRID(ncells) \
	((struct t_cell[ncells]){{0}})
#define T_SCRATCH_FRAME(ncells, Mwidth, Mheight) \
	((struct t_frame){ .grid = T_SCRATCH_GRID(ncells), .width = Mwidth, .height = Mheight, })

static inline struct t_cell *
t_frame_cell_at(
	struct t_frame *src,
	int32_t x,
	int32_t y
) {
	return &src->grid[(y * src->width) + x];
}

enum t_status
t_frame_init_pattern(
	struct t_frame *dst,
	char const *pattern
);

void
t_frame_compute_clip(
	struct t_box *dst,
	struct t_frame *src
);

enum t_status
t_frame_push_inset_clip(
	struct t_frame *dst,
	int32_t lx,
	int32_t ly,
	int32_t rx,
	int32_t ry
);

static inline void
t_frame_pop_clip(
	struct t_frame *src
) {
	if (src->_clip_index) {
		--src->_clip_index;
	}
}

/* +--- MAPPING ---------------------------------------------------+ */
#define T_MAP_CH         0x01
#define T_MAP_FOREGROUND 0x02
#define T_MAP_BACKGROUND 0x04

void
t_frame_map(
	struct t_frame *dst,
	uint32_t mode,
	uint32_t rgba_fg,
	uint32_t rgba_bg,
	char from,
	char to
);

void
t_frame_clear(
	struct t_frame *dst
);

void
t_frame_paint(
	struct t_frame *dst,
	uint32_t rgba_fg,
	uint32_t rgba_bg
);

static inline void
t_frame_cull(
	struct t_frame *dst,
	char what
) {
	t_frame_map(dst, ~(0), 0, 0, what, '\0');
}

/* +--- BLENDING --------------------------------------------------+ */
void
t_frame_overlay(
	struct t_frame *dst,
	struct t_frame *src,
	int32_t x,
	int32_t y
);

/* +--- RASTERIZING -----------------------------------------------+ */
enum t_status
t_frame_rasterize(
	struct t_frame *src,
	int32_t x,
	int32_t y
);

#endif /* INCLUDE_T_DRAW_H */
