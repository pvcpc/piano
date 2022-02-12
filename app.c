#include <stdlib.h>
#include <stdio.h>

#include <time.h>
#include <unistd.h>

#include "app.h"
#include "common.h"
#include "terminal.h"


static struct timespec g_genesis;

static bool g_should_run = true;

int
main(void)
{
	/*
	 * Setup
	 */
	if (!t_manager_setup()) {
		panic_and_die(1, "Not a TTY!");
	}

	clock_gettime(CLOCK_MONOTONIC, &g_genesis);

	/*
	 * Done with setup, any other code in the codebase can now use
	 * any function defined in `app.h` with complete confidence.
	 *
	 * Now the almighty event loop as per usual.
	 */
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

		t_cursor_pos(1, 1);
		t_writef("Current time: %.2f", seconds_since_genesis());
		t_flush();
	}

	/*
	 * Cleanup
	 */
	t_manager_cleanup();
	return 0;
}

double
seconds_since_genesis()
{
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);

	return   (now.tv_sec - g_genesis.tv_sec)
		   + (now.tv_nsec - g_genesis.tv_nsec) * 1e-9;
}

void
panic_and_die(u32 code, char const *message) 
{
	fputs(message, stderr);
	exit(code);
}
