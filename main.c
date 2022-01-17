#include <stdio.h>

#include "t_base.h"
#include "t_input.h"
#include "t_output.h"


int
main(void)
{
	t_setup();

	struct t_frame frame;
	t_frame_create(&frame, 256, 256);

	t_frame_draw_box(&frame, 0, 0, 16, 8);

	t_cleanup();
	return 0;
#if 0
	int32_t box_x = 0;
	int32_t box_y = 0; 

	uint32_t front = 0;
	uint32_t back = 1;

	struct t_frame fb [2];
	t_frame_create(&fb[front], 256, 256);
	t_frame_create(&fb[back], 256, 256);

	while (1) {
		struct t_event ev;
		t_event_clear(&ev);
		enum t_status stat = t_poll(&ev);

		uint32_t w, h;
		t_viewport_size_get(&w, &h);
		t_frame_resize(&fb[front], w, h);
		t_frame_resize(&fb[back], w, h);

		t_frame_clear(&fb[front]);
		t_frame_draw_box();

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

		front ^= 1;
		back ^= 1;
	}

	t_image_destroy(&fb);
	t_cleanup();
	return 0;
#endif
}
