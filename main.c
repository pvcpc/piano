#include <stdio.h>

#include "t_base.h"
#include "t_sequence.h"


int
main(void)
{
	t_setup();

	t_clear();
	t_cursor_pos(0, 0);
	t_foreground_256_ex(96, 144, 210);
	t_background_256_ex(210, 144, 96);
	t_write_z("Hello, world");
	t_flush();

	while (1) {
		int32_t poll_code = t_poll();

		if (poll_code >= 0) {
			printf("%04x\n", poll_code);
		}

		switch (poll_code) {
		case T_POLL_CODE(0, 'h'):
			puts("h was pressed");
			break;
		case T_POLL_CODE(0, 'j'):
			puts("j was pressed");
			break;
		case T_POLL_CODE(0, 'k'):
			puts("k was pressed");
			break;
		case T_POLL_CODE(0, 'l'):
			puts("l was pressed");
			break;
		}
	}

	t_cleanup();
	return 0;
}
