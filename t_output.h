#ifndef INCLUDE_T_OUTPUT_H
#define INCLUDE_T_OUTPUT_H

#include "t_util.h"

/* common, unparameterized sequences for quick & dirty use with t_writez */
#define TR_CLRSCRN     "\x1b[2J"

#define TR_CURSOR_SHOW "\x1b[?25h"
#define TR_CURSOR_HIDE "\x1b[?25l"

/* common, parameterized sequences for quick & dirty use with t_writef */
#define TR_CURSOR_POS  "\x1b[%d;%dH"


enum t_status
t_write(
	uint8_t const *data,
	uint32_t length
);

enum t_status
t_writez(
	char const *data
);

#endif /* INCLUDE_T_OUTPUT_H */
