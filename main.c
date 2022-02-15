#include "common.h"
#include "terminal.h"
#include "draw.h"
#include "app.h"


#ifndef APP_NO_MAIN
static bool g_should_run = true;

int
main(void)
{
	app_setup_and_never_call_again();

	while (g_should_run) {
		switch (t_poll()) {
		case T_POLL_CODE(0, 'a'):
			t_writez("Got a");
			break;
		case T_POLL_CODE(0, 'b'):
			t_writez("Got b");
			break;
		case T_POLL_CODE(0, 'c'):
			t_writez("Got c");
			break;
		case T_POLL_CODE(0, 'd'):
			t_writez("Got d");
			break;
		}
		t_flush();
	}

	app_cleanup_and_never_call_again();
	return 0;
}
#endif /* APP_NO_MAIN */
