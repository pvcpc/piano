#include <termios.h>
#include <unistd.h>
#include <poll.h>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "t_base.h"


int
main(void)
{
	t_setup();

	while (1) {
		struct t_event ev;
		t_event_clear(&ev);
		t_poll(&ev);

		switch (ev.qcode) {
		case T_QCODE(0, 'h'):
			t_writez(TR_FG_RED "h was pressed");
			break;
		case T_QCODE(0, 'j'):
			t_writez(TR_FG_GREEN "j was pressed");
			break;
		case T_QCODE(0, 'k'):
			t_writez(TR_FG_YELLOW "k was pressed");
			break;
		case T_QCODE(0, 'l'):
			t_writez(TR_FG_BLUE "l was pressed");
			break;
		}
	}

	t_cleanup();
	return 0;
}
