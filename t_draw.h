#ifndef INCLUDE_T_DRAW_H
#define INCLUDE_T_DRAW_H

#include "t_util.h"
#include "t_sequence.h"

/* compile-time constants (see README):
 * - TC_CELL_BLOCK_WIDTH (default 16):
 *   Set the chunk size by which `t_frame` width will actually be
 *   allocated to minimize malloc(3)/free(3) calls when resizing.
 *
 * - TC_CELL_BLOCK_HEIGHT (default 16):
 *   Set the chunk size by which `t_frame` height will actually be
 *   allocated to minimize malloc(3)/free(3) calls when resizing.
 */
#ifndef TC_CELL_BLOCK_WIDTH
#  define TC_CELL_BLOCK_WIDTH 16
#endif

#ifndef TC_CELL_BLOCK_HEIGHT
#  define TC_CELL_BLOCK_HEIGHT 16
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

uint8_t
t_rgb_compress_cube_256(
	uint32_t rgb
);

uint8_t
t_rgb_compress_gray_256(
	uint32_t rgb
);

uint8_t
t_rgb_compress_256(
	uint32_t rgb
);

#define t_foreground_256_rgba(rgba) \
	t_foreground_256(t_rgb_compress_256(rgba))
#define t_background_256_rgba(rgba) \
	t_background_256(t_rgb_compress_256(rgba))

#define t_foreground_256_ex(r, g, b) \
	t_foreground_256_rgba(T_RGB(r, g, b))
#define t_background_256_ex(r, g, b) \
	t_background_256_rgba(T_RGB(r, g, b))

/* +--- MATH & MISC. COMPUTATION ----------------------------------+ */
struct t_box
{
	int32_t x0;
	int32_t y0;
	int32_t x1;
	int32_t y1;
};

#define T_BOX(x0, y0, x1, y1) \
	((struct t_box){ x0, y0, x1, y1 })
#define T_BOX_GEOM(x, y, width, height) \
	((struct t_box){ x, y, x + width, y + height })
#define T_BOX_SCREEN(width, height) \
	((struct t_box){ 0, 0, width, height })

static inline struct t_box *
t_box_standardize(
	struct t_box *dst
) {
	int32_t
		x0 = T_MIN(dst->x0, dst->x1),
		y0 = T_MIN(dst->y0, dst->y1),
		x1 = T_MAX(dst->x0, dst->x1),
		y1 = T_MAX(dst->y0, dst->y1);
	dst->x0 = x0;
	dst->y0 = y0;
	dst->x1 = x1;
	dst->y1 = y1;
	return dst;
}

static inline struct t_box * 
t_box_intersect_no_standardize(
	struct t_box *dst,
	struct t_box *src
) {
	int32_t
		x0 = T_MAX(dst->x0, src->x0),
		y0 = T_MAX(dst->y0, src->y0),
		x1 = T_MIN(dst->x1, src->x1),
		y1 = T_MIN(dst->y1, src->y1);
	dst->x0 = x0;
	dst->y0 = y0;
	dst->x1 = x1;
	dst->y1 = y1;
	return dst;
}

static inline struct t_box *
t_box_intersect(
	struct t_box *dst,
	struct t_box *src
) {
	return t_box_intersect_no_standardize(
		t_box_standardize(dst),
		t_box_standardize(src)
	);
}

/* +--- FRAME DRAWING ---------------------------------------------+ */
struct t_cell
{
	uint8_t ch;

	/* see t_sequence.h for how color is packed */
	uint32_t fg_rgba; 
	uint32_t bg_rgba;
};

struct t_frame
{
	struct t_cell *grid;
	int32_t width;
	int32_t height;

	/* persistent blend settings (reset with `t_frame_blend_reset()`) */
	struct {
		struct t_box clip;
	} blend;

	/* internal */
	uint32_t _true_width;
	uint32_t _true_height;
};

enum t_frame_flag
{
	T_FRAME_SPACEHOLDER  = 0x01,
};

enum t_blend_flag
{
	/* destination/source sampling mode: if set, sample from source;
	 * if not set, sample from destination. */
	T_BLEND_R            = 0x0001,
	T_BLEND_G            = 0x0002,
	T_BLEND_B            = 0x0004,
	T_BLEND_A            = 0x0008,
	T_BLEND_CH           = 0x0010,

	/* source foreground/background color from additional 
	 * flat_fg/bg_rgb parameters instead of either destination or
	 * source. */
	T_BLEND_FGOVERRIDE   = 0x0100,
	T_BLEND_BGOVERRIDE   = 0x0200,

	/* for ease of use */
	T_BLEND_ALL          = 0xffff,
};

enum t_status
t_frame_create(
	struct t_frame *dst,
	int32_t width,
	int32_t height
);

enum t_status
t_frame_create_pattern(
	struct t_frame *dst,
	enum t_frame_flag flags,
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

enum t_status
t_frame_clear(
	struct t_frame *dst
);

enum t_status
t_frame_paint(
	struct t_frame *dst,
	int32_t fg_rgb,
	int32_t bg_rgb
);

enum t_status
t_frame_blend_reset(
	struct t_frame *dst
);

enum t_status
t_frame_blend_set_clip(
	struct t_frame *dst,
	struct t_box *box
);

enum t_status
t_frame_blend(
	struct t_frame *dst,
	struct t_frame *src,
	enum t_blend_flag flags,
	int32_t flat_fg_rgb,
	int32_t flat_bg_rgb,
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
