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

#define T_BOX_WIDTH(box_ptr) \
	T_ABS((box_ptr)->x1 - (box_ptr)->x0)
#define T_BOX_HEIGHT(box_ptr) \
	T_ABS((box_ptr)->y1 - (box_ptr)->y0)

static inline struct t_box *
t_box_standardize(
	struct t_box *dst,
	struct t_box *src
) {
	int32_t
		x0 = T_MIN(src->x0, src->x1),
		y0 = T_MIN(src->y0, src->y1),
		x1 = T_MAX(src->x0, src->x1),
		y1 = T_MAX(src->y0, src->y1);
	dst->x0 = x0;
	dst->y0 = y0;
	dst->x1 = x1;
	dst->y1 = y1;
	return dst;
}

static inline struct t_box * 
t_box_intersect_no_standardize(
	struct t_box *dst,
	struct t_box *boxa,
	struct t_box *boxb
) {
	dst->x0 = T_MAX(boxa->x0, boxb->x0),
	dst->y0 = T_MAX(boxa->y0, boxb->y0),
	dst->x1 = T_MIN(boxa->x1, boxb->x1),
	dst->y1 = T_MIN(boxa->y1, boxb->y1);
	return dst;
}

static inline struct t_box *
t_box_intersect(
	struct t_box *dst,
	struct t_box *boxa,
	struct t_box *boxb
) {
	struct t_box tmpa, tmpb;
	return t_box_intersect_no_standardize(dst,
		t_box_standardize(&tmpa, boxa),
		t_box_standardize(&tmpb, boxb)
	);
}

static inline struct t_box *
t_box_translate(
	struct t_box *dst,
	struct t_box *src,
	int32_t x,
	int32_t y
) {
	dst->x0 = src->x0 + x;
	dst->y0 = src->y0 + y;
	dst->x1 = src->x1 + x;
	dst->y1 = src->y1 + y;
	return dst;
}

/* +--- FRAME DRAWING ---------------------------------------------+ */
enum t_gravity
{
	/* x axis mnemonic */
	T_GRAVITY_LEFT      = 0,
	T_GRAVITY_RIGHT     = 2,

	/* y axis mnemonic */
	T_GRAVITY_TOP       = 0,
	T_GRAVITY_BOTTOM    = 2,

	/* axis agnostic */
	T_GRAVITY_CENTER    = 1
};

enum t_alignment
{
	/* x axis mnemonic */
	T_ALIGNMENT_LEFT    =  0,
	T_ALIGNMENT_RIGHT   = -2,

	/* y axis mnemonic */
	T_ALIGNMENT_TOP     =  0,
	T_ALIGNMENT_BOTTOM  = -2,

	/* axis agnostic mnemonic */
	T_ALIGNMENT_CENTER  = -1
};

enum t_direction
{
	/* x axis mnemonic (left to right, right to left) */
	T_DIRECTION_L2R     =  1,
	T_DIRECTION_R2L     = -1,

	/* y axis mnemonic (top to bottom, bottom to top) */
	T_DIRECTION_T2B     =  1,
	T_DIRECTION_B2T     = -1,
};

struct t_coordinate_system
{
	/* multipliers (see `enum t_gravity`, `enum t_alignment`, 
	 * `enum t_direction`) */
	int8_t  gravity;
	int8_t  alignment;
	int8_t  direction;
	int32_t origin;
};

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

	/* context (reset with `t_frame_context_reset()`) */
	struct {
		struct t_box clip;
		struct t_coordinate_system x;
		struct t_coordinate_system y;
	} context;

	/* internal */
	uint32_t _true_width;
	uint32_t _true_height;
};

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

enum t_typeset_flag
{
	T_TYPESET_ELLIPSES    = 0x0001,
	T_TYPESET_WRAP        = 0x0002,
	T_TYPESET_FLUSH_LEFT  = 0x0004,
	T_TYPESET_FLUSH_RIGHT = 0x0008,

	/* override the color wherever text is placed with the 
	 * foreground/background specified in the parameters.
	 */
	T_TYPESET_FGOVERRIDE  = 0x0010,
	T_TYPESET_BGOVERRIDE  = 0x0020,

	/* whether to clamp the text bounding box into the frame (if not
	 * clamped, text may clip out of the frame.) */
	T_TYPESET_CLAMP_BOX   = 0x0100,
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
t_frame_context_set_gravity(
	struct t_frame *dst,
	enum t_gravity xgrav,
	enum t_gravity ygrav
);

enum t_status
t_frame_context_set_alignment(
	struct t_frame *dst,
	enum t_alignment xalign,
	enum t_alignment yalign
);

enum t_status
t_frame_context_set_direction(
	struct t_frame *dst,
	enum t_direction xdir,
	enum t_direction ydir
);

enum t_status
t_frame_context_set_origin(
	struct t_frame *dst,
	int32_t x,
	int32_t y
);

enum t_status
t_frame_context_reset_clip(
	struct t_frame *dst
);

enum t_status
t_frame_context_reset_coordinate_system(
	struct t_frame *dst
);

enum t_status
t_frame_context_reset_everything(
	struct t_frame *dst
);

enum t_status
t_frame_clear(
	struct t_frame *dst
);

enum t_status
t_frame_paint(
	struct t_frame *dst,
	uint32_t fg_rgb,
	uint32_t bg_rgb
);

enum t_status
t_frame_map_one(
	struct t_frame *dst,
	enum t_map_flag flags,
	uint32_t alt_fg_rgba,
	uint32_t alt_bg_rgba,
	char from,
	char to
);

enum t_status
t_frame_blend(
	struct t_frame *dst,
	struct t_frame *src,
	enum t_blend_mask mask,
	enum t_blend_flag flags,
	uint32_t alt_fg_rgb,
	uint32_t alt_bg_rgb,
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
