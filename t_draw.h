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

/* +--- COLOR UTILITIES -------------------------------------------+ */
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

/* +--- FRAME DRAWING ---------------------------------------------+ */
struct t_cell
{
	/* see t_sequence.h for how color is packed */
	uint32_t rgba_fg; 
	uint32_t rgba_bg;
	uint8_t  ch;
};

struct t_frame
{
	struct t_cell *grid;
	int32_t width;
	int32_t height;
};

#if 0
enum t_frame_flag
{
	T_FRAME_SPACEHOLDER   = 0x01,
};

enum t_map_flag
{
	T_MAP_CH              = 0x0001,
	T_MAP_ALTFG           = 0x0002,
	T_MAP_ALTBG           = 0x0004,
};

enum t_blend_mask
{
	/* destination/source sampling mode: if set, sample from source;
	 * if not set, sample from destination. */
	T_BLEND_R             = 0x0001,
	T_BLEND_G             = 0x0002,
	T_BLEND_B             = 0x0004,
	T_BLEND_A             = 0x0008,
	T_BLEND_CH            = 0x0010,
};

enum t_blend_flag
{
	T_BLEND_ALTFG         = 0x0001,
	T_BLEND_ALTBG         = 0x0002,
};
#endif

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
t_frame_overlay(
	struct t_frame *dst,
	struct t_frame *src,
	int32_t x,
	int32_t y
);

enum t_status
t_frame_rasterize(
	struct t_frame *src,
	int32_t x,
	int32_t y
);

#endif /* INCLUDE_T_DRAW_H */
