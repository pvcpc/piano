#ifndef INCLUDE_T_DRAW_H
#define INCLUDE_T_DRAW_H

#include "t_util.h"

/* compile-time constants (see README):
 * - TC_CELL_WIDTH_BLOCK (default 16):
 *   Set the chunk size by which `t_frame` width will actually be
 *   allocated to minimize malloc(3)/free(3) calls when resizing.
 *
 * - TC_CELL_HEIGHT_BLOCK (default 16):
 *   Set the chunk size by which `t_frame` height will actually be
 *   allocated to minimize malloc(3)/free(3) calls when resizing.
 */
#ifndef TC_CELL_WIDTH_BLOCK
#  define TC_CELL_WIDTH_BLOCK 16
#endif

#ifndef TC_CELL_HEIGHT_BLOCK
#  define TC_CELL_HEIGHT_BLOCK 16
#endif

/* +--- FRAME DRAWING ---------------------------------------------+ */
struct t_cell
{
	uint8_t ch;
	struct {
		uint8_t r, g, b;
	} fg;
	struct {
		uint8_t r, g, b;
	} bg;
};

struct t_frame
{
	struct t_cell *grid;
	uint32_t width;
	uint32_t height;

	uint32_t _block_width;
	uint32_t _block_height;
};

enum t_frame_flag
{
	T_FRAME_SPACEHOLDER = 0x01,
};

enum t_status
t_frame_create(
	struct t_frame *dst,
	uint32_t width,
	uint32_t height
);

enum t_status
t_frame_create_pattern(
	struct t_frame *dst,
	char const *pattern,
	enum t_frame_flag flags
);

void
t_frame_destroy(
	struct t_frame *dst
);

enum t_status
t_frame_resize(
	struct t_frame *dst,
	uint32_t n_width,
	uint32_t n_height
);

enum t_status
t_frame_paint(
	struct t_frame *dst,
	int fg_rgb,
	int bg_rgb
);

enum t_status
t_frame_rasterize(
	struct t_frame *src,
	int32_t x,
	int32_t y
);

/* +--- DIRECT DRAWING --------------------------------------------+ */

#endif /* INCLUDE_T_DRAW_H */
