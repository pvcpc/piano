#include <stdio.h>

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
			puts("h was pressed");
			break;
		case T_QCODE(0, 'j'):
			puts("j was pressed");
			break;
		case T_QCODE(0, 'k'):
			puts("k was pressed");
			break;
		case T_QCODE(0, 'l'):
			puts("l was pressed");
			break;
		}
	}

	t_cleanup();
	return 0;
}
