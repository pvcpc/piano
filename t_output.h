#ifndef INCLUDE_T_OUTPUT_H
#define INCLUDE_T_OUTPUT_H

#include "t_util.h"


/* direct write */
enum t_status
t_write(
	uint8_t const *data,
	uint32_t length
);

enum t_status
t_writez(
	char const *data
);

/* cpu-side, ascii image rendering */
#define T_FRAME_WIDTH_BLOCK 16
#define T_FRAME_HEIGHT_BLOCK 16

struct t_cell
{
	uint16_t letter     : 8,
	         foreground : 3,
	         background : 3;
};

struct t_frame
{
	struct t_cell *cells;
	uint32_t w;
	uint32_t h;

	uint32_t _true_w; /* true width of the frame */
	uint32_t _true_h; /* true height of the frame */
};

#endif /* INCLUDE_T_OUTPUT_H */
