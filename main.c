#include <stdio.h>

#include "t_base.h"
#include "t_input.h"
#include "t_output.h"


int
main(void)
{
	t_setup();

	while (1) {
		struct t_event ev;
		t_event_clear(&ev);

		enum t_status stat = t_poll(&ev);

		if (stat >= 0) {
			printf("elapsed %.2fs — delta %.2fs\n",
				t_elapsed(), t_delta());
		}

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
		case T_QCODE(T_TIMER, 0):
			printf("timer 0: delta %.2fs, elapsed %.2fs\n", ev.delta, ev.elapsed);
			break;
		case T_QCODE(T_TIMER, 1):
			printf("timer 1: delta %.2fs, elapsed %.2fs\n", ev.delta, ev.elapsed);
			break;
		}
	}

	t_cleanup();
	return 0;
}
