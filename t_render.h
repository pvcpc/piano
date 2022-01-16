#ifndef INCLUDE_T_RENDER_H
#define INCLUDE_T_RENDER_H

#include "t_util.h"

/* common, unparameterized sequences for quick & dirty use with t_writez */
#define TR_CLRSCRN     "\x1b[2J"

#define TR_CURSOR_SHOW "\x1b[?25h"
#define TR_CURSOR_HIDE "\x1b[?25l"

/* common, parameterized sequences for quick & dirty use with t_writef */
#define TR_CURSOR_POS  "\x1b[%d;%dH"


/* For use to build one large render string to be sent in one call to
 * `t_write`.
 */
struct t_render_buffer
{
	uint8_t *base;
	uint8_t *end;
	uint8_t *cursor;
};

enum t_status
t_render_buffer_init(
	struct t_render_buffer *buffer
);

void
t_render_buffer_destroy(
	struct t_render_buffer *buffer
);


#endif /* INCLUDE_T_RENDER_H */
