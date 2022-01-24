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
	char ch;

	/* see t_sequence.h for how color is packed */
	int32_t fg_rgb; 
	int32_t bg_rgb;
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
	/* pattern flags */
	T_FRAME_SPACEHOLDER = 0x0010,

	/* blend flags */
	T_FRAME_DSTOVER     = 0x0100,
	T_FRAME_SRCOVER     = 0x0200,
	T_FRAME_PAINTOVER   = 0x0400,
	T_FRAME_PAINTWASHED = 0x0800,
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
	int fg_rgb,
	int bg_rgb
);

enum t_status
t_frame_wash(
	struct t_frame *dst
);

enum t_status
t_frame_blend(
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
