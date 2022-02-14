#include <stdlib.h>
#include <stdio.h>

#include <time.h>
#include <unistd.h>

#include "common.h"
#include "terminal.h"
#include "draw.h"
#include "app.h"

/* @SECTION(app) */

/* @GLOBAL */
static struct timespec g_genesis;

static bool g_should_run = true;

double
app_sleep(double seconds)
{
	struct timespec start;
	clock_gettime(CLOCK_MONOTONIC, &start);

	u64 nanoseconds = (u64) (seconds * 1e9);
	struct timespec sleep = {
		.tv_sec  = nanoseconds / APP__NANO,
		.tv_nsec = nanoseconds % APP__NANO,
	};
	clock_nanosleep(CLOCK_MONOTONIC, 0, &sleep, NULL);

	struct timespec end;
	clock_gettime(CLOCK_MONOTONIC, &end);

	return ( 
		(end.tv_sec - start.tv_sec) +
		(end.tv_nsec - start.tv_nsec) * 1e-9
	);
}

double
app_seconds_since_genesis()
{
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);

	return   (now.tv_sec - g_genesis.tv_sec)
		   + (now.tv_nsec - g_genesis.tv_nsec) * 1e-9;
}

void
app_panic_and_die(u32 code, char const *message) 
{
	fputs(message, stderr);
	exit(code);
}

/* @SECTION(main) */
int
main(void)
{
	/*
	 * Setup
	 */
	if (!t_manager_setup()) {
		app_panic_and_die(1, "Not a TTY!");
	}

	clock_gettime(CLOCK_MONOTONIC, &g_genesis);

	/*
	 * Done with setup, any other code in the codebase can now use
	 * any function defined in `app.h` with complete confidence.
	 *
	 * Now the almighty event loop as per usual.
	 */
	while (g_should_run) {

		struct frame frame = LOCAL_FRAME(4, 4);
		frame_zero_grid(&frame);
		frame_load_pattern(&frame, 0, 0,
			"+--+\n"
			"|  |\n"
			"|  |\n"
			"+--+\n"
		);

		t_clear();
		t_cursor_pos(1, 1);
		frame_rasterize(&frame, 0, 0);

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

		app_sleep(8e-3);
		t_flush();
	}

	/*
	 * Cleanup
	 */
	t_manager_cleanup();
	return 0;
}
