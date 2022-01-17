#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include <string.h>
#include <time.h>

#include "t_base.h"


/* @GLOBAL */
static struct termios         g_tios_old;

static struct timespec        g_time_genesis;
static struct timespec        g_time_previous;

void
t_setup()
{
	/* disable "canonical mode" so we can read input as it's typed. We
	 * get two copies so we can restore the previous settings at he end 
	 * of the program.
	 */
	struct termios now;
	tcgetattr(STDIN_FILENO, &g_tios_old);
	tcgetattr(STDIN_FILENO, &now);

	now.c_lflag &= ~(ICANON | ECHO);
	now.c_cc[VTIME] = 0; /* min of 0 deciseconds for read(3) to block */
	now.c_cc[VMIN] = 0; /* min of 0 characters for read(3) */
	tcsetattr(STDIN_FILENO, TCSANOW, &now);

	/* time stuff */
	clock_gettime(CLOCK_MONOTONIC, &g_time_genesis);
	clock_gettime(CLOCK_MONOTONIC, &g_time_previous);
}

void
t_cleanup()
{
	/* reset terminal settings back to normal */
	tcsetattr(STDIN_FILENO, TCSANOW, &g_tios_old);
}

double
t_elapsed()
{
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);

	return 1e-9 * (
		(now.tv_sec - g_time_genesis.tv_sec) * 1e9 +
		(now.tv_nsec - g_time_genesis.tv_nsec)
	);
}

double
t_delta()
{
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);

	double const delta = 1e-9 * (
		(now.tv_sec - g_time_previous.tv_sec) * 1e9 +
		(now.tv_nsec - g_time_previous.tv_nsec)
	);

	g_time_previous.tv_sec = now.tv_sec;
	g_time_previous.tv_nsec = now.tv_nsec;

	return delta;
}

enum t_status
t_viewport_size_get(
	uint32_t *out_w,
	uint32_t *out_h
) {
	struct winsize ws;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) < 0) return T_EUNKNOWN;

	if (out_w) *out_w = ws.ws_col;
	if (out_h) *out_h = ws.ws_row;

	return T_OK;
}
