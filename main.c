#include "t_base.h"
#include "t_graphics.h"


int
main(void)
{
	t_setup();
	t_graphics_setup();

	while (1) {
		struct t_event ev;
		t_event_clear(&ev);
		t_poll(&ev);

		t_frame_buffer_size_to_viewport();
		t_frame_buffer_clear();

		switch (ev.qcode) {
		case T_QCODE(0, 'h'):
			t_writez("h was pressed\n");
			break;
		case T_QCODE(0, 'j'):
			t_writez("j was pressed\n");
			break;
		case T_QCODE(0, 'k'):
			t_writez("k was pressed\n");
			break;
		case T_QCODE(0, 'l'):
			t_writez("l was pressed\n");
			break;
		}
	}

	t_graphics_cleanup();
	t_cleanup();
	return 0;
}
