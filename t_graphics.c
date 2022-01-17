#include <sys/ioctl.h>
#include <unistd.h>

#include "t_graphics.h"


/* @GLOBAL */

enum t_status
t_viewport_size_get(
	uint32_t *out_w,
	uint32_t *out_h
) {
	struct winsize ws;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) < 0) return T_EUNKNOWN;

	if (out_w) *out_w = ws.ws_col;
	if (out_h) *out_h = ws.ws_row;

	return T_OK;
}

