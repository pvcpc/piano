#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "t_base.h"
#include "t_render.h"


int
main(void)
{
	t_setup();

	t_writez(TR_CLRSCRN);
	t_writef(TR_CURSOR_POSITION, 0, 0);

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
