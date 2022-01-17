#include <unistd.h>

#include <string.h>

#include "t_output.h"


enum t_status
t_write(
	uint8_t const *data,
	uint32_t length
) {
	if (!data) return T_ENULL;

	/* @TODO(max): sometime in the future get errno to report proper
	 * error, although, with stdout, that's not usually an issue. */
	return write(STDOUT_FILENO, data, length) < 0 ? T_EUNKNOWN : T_OK;
}

enum t_status
t_writez(
	char const *data
) {
	if (!data) return T_ENULL;

	return t_write((uint8_t const *) data, strlen(data));
}
