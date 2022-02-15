#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <time.h>

#include "common.h"
#include "terminal.h"
#include "app.h"

/* @SECTION(app) */
#define APP__NANO 1000000000

/* @GLOBAL */
static struct timespec g_genesis;

static bool            g_should_run = true;

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

#ifdef APP_DEMO
extern int
demo();
#endif

int
main(void)
{
	/* 
	 * Setup
	 */
	if (!t_manager_setup()) {
		app_panic_and_die(1, "Not a TTY!");
	}

	if (clock_gettime(CLOCK_MONOTONIC, &g_genesis) < 0) {
		app_panic_and_die(1, "Check your clock captain!");
	}

	/*
	 * Do the cool stuff
	 */
#ifdef APP_DEMO
	demo();
#else
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
#endif

	t_manager_cleanup();
	return 0;
}
