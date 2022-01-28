#ifndef INCLUDE_T_DRAW_H
#define INCLUDE_T_DRAW_H

#include "t_util.h"

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
