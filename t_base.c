#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <poll.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "t_base.h"

#define T__POLL_CODES_MAX 16
#define T__PARAMS_MAX 4
#define T__INTERS_MAX 4
#define T__SCRATCH_BUFFER_SIZE 256

#define T__NANO_PER_SEC 1000000000


enum t_poll_code
{
	/* parsing codes */
	TM_DETERMINE,

	TM_NORMAL,
	TM_CONTROL,

	TM_ESCAPE,
	TM_ESCAPE_BRACKET,

	TM_SS3_FUNC,

	TM_PARAM,
	TM_PARAM_EMIT,

	TM_INTER,

	/* state change codes */
	TM_RESET,
	TM_READ,

	/* halt codes */
	TM_HALT_UNKNOWN,
	TM_HALT_IF_EMPTY,
	TM_HALT_EMIT,
};

/* @GLOBAL */
/* https://invisible-island.net/xterm/ctlseqs/ctlseqs.html#h3-PC-Style-Function-Keys */
/* @NOTE(max): the way T_* keys are assigned, this table is actually just
 * an identity table, so it may be removed if the modifiers are not
 * reassigned later. */
static enum t_poll_mod const PC_KEYMOD_TABLE [] = {
	[ 2] =      0 |         0 |     0 | T_SHIFT,
	[ 3] =      0 |         0 | T_ALT |       0,
	[ 4] =      0 |         0 | T_ALT | T_SHIFT,
	[ 5] =      0 | T_CONTROL |     0 |       0,
	[ 6] =      0 | T_CONTROL |     0 | T_SHIFT,
	[ 7] =      0 | T_CONTROL | T_ALT |       0,
	[ 8] =      0 | T_CONTROL | T_ALT | T_SHIFT,
	[ 9] = T_META |         0 |     0 |       0,
	[10] = T_META |         0 |     0 | T_SHIFT,
	[11] = T_META |         0 | T_ALT |       0, 
	[12] = T_META |         0 | T_ALT | T_SHIFT,
	[13] = T_META | T_CONTROL |     0 |       0,
	[14] = T_META | T_CONTROL |     0 | T_SHIFT,
	[15] = T_META | T_CONTROL | T_ALT |       0,
	[16] = T_META | T_CONTROL | T_ALT | T_SHIFT,
};

static enum t_poll_special const PC_KEYSPEC_TABLE [] = {
/*
	[  1] = T_HOME,
	[  2] = T_INSERT,
	[  3] = T_DELETE,
	[  4] = T_END,
	[  5] = T_PAGE_UP,   
	[  6] = T_PAGE_DOWN,
	[  7] = T_HOME,
	[  8] = T_END,
*/
	[ 10] = T_F0,
	[ 11] = T_F1,
	[ 12] = T_F2,
	[ 13] = T_F3,
	[ 14] = T_F4,
	[ 15] = T_F5,
	[ 17] = T_F6,
	[ 18] = T_F7,
	[ 19] = T_F8,
	[ 20] = T_F9,
	[ 21] = T_F10,
	[ 23] = T_F11,
	[ 24] = T_F12,
};

/* @GLOBAL */
static struct termios         g_tios_old;
static struct timespec        g_time_genesis;

/* poll machine */
static uint8_t                g_read_buf     [TC_GLOBAL_READ_BUFFER_SIZE];
static uint8_t const         *g_read_cursor;
static uint8_t const         *g_read_end;
static int                    g_read_buf_len;

static enum t_poll_code       g_codes        [T__POLL_CODES_MAX];
static uint32_t               g_code_p;

static uint16_t               g_params       [T__PARAMS_MAX];
static uint32_t               g_param_p;

static uint8_t                g_inters       [T__INTERS_MAX];
static uint32_t               g_inter_p;

static uint8_t                g_scratch      [T__SCRATCH_BUFFER_SIZE];
static uint32_t               g_scratch_p;

static uint8_t                g_mod;
static uint8_t                g_val;
static bool                   g_is_escape;

/* @SPEED(max): check g_write_buf alignment for vectorized memcpy */
static uint8_t                g_write_buf    [TC_GLOBAL_WRITE_BUFFER_SIZE];
static uint8_t               *g_write_cursor = g_write_buf;
static uint8_t const * const  g_write_end    = g_write_buf + TC_GLOBAL_WRITE_BUFFER_SIZE;

static char                  *g_write_f_buf;
static uint32_t               g_write_f_buf_size;

#ifdef TC_DEBUG_METRICS
static uint32_t               g_debug_write_nflushed;
static uint32_t               g_debug_write_nstored;
static uint32_t               g_debug_write_f_nstored;
#endif

/* +--- GENERAL ---------------------------------------------------+ */
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
}

void
t_cleanup()
{
	/* flush any remaining output in case things like `t_cursor_show()`
	 * are called at the end to restore defaults.
	 */
	t_flush();

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
t_sleep(
	double seconds
) {
	struct timespec start;
	clock_gettime(CLOCK_MONOTONIC, &start);

	uint64_t nanoseconds = (uint64_t) (seconds * 1e9);
	struct timespec sleep = {
		.tv_sec  = nanoseconds / T__NANO_PER_SEC,
		.tv_nsec = nanoseconds % T__NANO_PER_SEC,
	};
	clock_nanosleep(CLOCK_MONOTONIC, 0, &sleep, NULL);

	struct timespec end;
	clock_gettime(CLOCK_MONOTONIC, &end);

	double r = ( 
		(end.tv_sec - start.tv_sec) +
		(end.tv_nsec - start.tv_nsec) * 1e-9
	);
	return r;
}

enum t_status
t_termsize(
	int32_t *out_width,
	int32_t *out_height
) {
	struct winsize ws;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) < 0) {
		return T_EUNKNOWN;
	}

	if (out_width) *out_width = (int32_t) ws.ws_col;
	if (out_height) *out_height = (int32_t) ws.ws_row;

	return T_OK;
}

/* +--- INPUT -----------------------------------------------------+ */
static enum t_poll_code
t_mach_push(
	enum t_poll_code code
) {
	/* @TODO(max): overflow not protected, need proper error handling */
	return g_codes[g_code_p++] = code;
}

static enum t_poll_code
t_mach_push_init(
	enum t_poll_code code
) {
	/* @NOTE(max): this was a common idiom in the poll machine below,
	 * and most codes using it were postfixed with "_INIT," hence the
	 * function name.
	 *
	 * @TODO(max): this function may be confusing when writing out
	 * case statements in the actual machine since there is no 
	 * indication that a code depends on this type of form.
	 */
	t_mach_push(code);
	if (g_read_cursor == g_read_end) {
		t_mach_push(TM_READ);
	}
	return code;
}

static enum t_poll_code
t_mach_pop() 
{
	/* @TODO(max): underflow not protected, need proper error handling */
	return g_codes[--g_code_p];
}

static uint8_t
t_mach_cursor_inc() 
{
	/* @TODO(max): overflow not protected, bad? */
	return *g_read_cursor++;
}

int32_t
t_poll() 
{
	t_mach_push(TM_RESET);

	while (g_code_p > 0) {
		enum t_poll_code const code      = t_mach_pop();
		uint32_t         const available = g_read_cursor <= g_read_end ? 
			g_read_end - g_read_cursor : 0;

		switch (code) {
		case TM_DETERMINE:
			if (!available) {
				t_mach_push(code);
				t_mach_push(TM_HALT_IF_EMPTY);
				t_mach_push(TM_READ);
			}
			else if (g_read_cursor[0] == '\x1b') {
				t_mach_push_init(TM_ESCAPE);
				t_mach_cursor_inc();
			}
			else if (0x00 <= g_read_cursor[0] &&
			                 g_read_cursor[0] <= 0x1f) {
				t_mach_push(TM_CONTROL);
			}
			else {
				t_mach_push(TM_NORMAL);
			}
			break;

		/* normal byte */
		case TM_NORMAL:
			t_mach_push(TM_HALT_EMIT);
			g_val = t_mach_cursor_inc();
			break;

		/* control byte */
		case TM_CONTROL:
			t_mach_push(TM_HALT_EMIT);
			g_mod |= T_CONTROL;
			g_val = t_mach_cursor_inc() | 0x40;
			break;

		/* escape sequence parser */
		case TM_ESCAPE:
			g_is_escape = 1;
			if (!available) {
				g_val = '\x1b';
				t_mach_push(TM_HALT_EMIT);
			}
			else if (g_read_cursor[0] == '\x1b') {
				g_val = '\x1b';
				t_mach_push(TM_HALT_EMIT);
				t_mach_cursor_inc();
			}
			else if (g_read_cursor[0] == '\x4f') {
				t_mach_push_init(TM_SS3_FUNC);
				t_mach_cursor_inc();
			}
			else if (g_read_cursor[0] == '[') {
				t_mach_push_init(TM_ESCAPE_BRACKET);
				t_mach_cursor_inc();
			}
			else {
				g_mod |= T_ALT;
				g_val = t_mach_cursor_inc();
				t_mach_push(TM_HALT_EMIT);
			}
			break;

		case TM_SS3_FUNC:
			if (!available) {
				g_val = '\x4f';
				t_mach_push(TM_HALT_EMIT);
				t_mach_cursor_inc();
			}
			else {
				g_mod |= T_SPECIAL;
				g_val = t_mach_cursor_inc() - '\x4f';
				t_mach_push(TM_HALT_EMIT);
			}
			break;

		case TM_ESCAPE_BRACKET:
			if (!available) {
				g_mod |= T_ALT;
				g_val = '[';
				t_mach_push(TM_HALT_EMIT);
			}
			/* @NOTE(max): we can't just follow TM_PARAM -> TM_INTER 
			 * -> TM_NORMAL because TM_PARAM will emit a parameter 
			 * even if there are none present, so we explicitly check 
			 * for the required byte ranges. */
			else if (0x30 <= g_read_cursor[0] &&
			                 g_read_cursor[0] <= 0x3f) {
				t_mach_push(TM_PARAM);
			}
			else if (0x20 <= g_read_cursor[0] &&
			                 g_read_cursor[0] <= 0x2f) {
				t_mach_push(TM_INTER);
			}
			else {
				t_mach_push(TM_NORMAL);
			}
			break;

		/* parameter parser */
		case TM_PARAM:
			if (!available) {
				t_mach_push(code);
				t_mach_push(TM_HALT_IF_EMPTY);
				t_mach_push(TM_READ);
			}
			else if (g_read_cursor[0] < 0x30 ||
			                       0x3f < g_read_cursor[0]) {
				t_mach_push(TM_INTER);
				t_mach_push(TM_PARAM_EMIT);
			}
			else if (g_read_cursor[0] == ';') {
				t_mach_push(code);
				t_mach_push(TM_PARAM_EMIT);
				t_mach_cursor_inc();
			}
			else {
				t_mach_push(code);
				if (g_scratch_p < T__SCRATCH_BUFFER_SIZE) {
					g_scratch[g_scratch_p++] = t_mach_cursor_inc();
				}
			}
			break;

		case TM_PARAM_EMIT:
			if (g_param_p < T__PARAMS_MAX) {
				uint16_t result = 0;
				for (uint32_t i = 0; i < g_scratch_p; ++i) {
					result *= 10;
					result += g_scratch[i] - '0';
				}
				g_params [g_param_p++] = result;
				g_scratch_p = 0;
			}
			break;

		/* intermediate bytes */
		case TM_INTER:
			if (!available) {
				t_mach_push(code);
				t_mach_push(TM_HALT_IF_EMPTY);
				t_mach_push(TM_READ);
			}
			else if (g_read_cursor[0] < 0x20 ||
			                       0x2f < g_read_cursor[0]) {
				t_mach_push(TM_NORMAL);
			}
			else {
				t_mach_push(code);
				if (g_inter_p < T__INTERS_MAX) {
					g_inters[g_inter_p++] = t_mach_cursor_inc();
				}
			}
			break;

		/* state updates */
		case TM_RESET:
			g_code_p = 0;
			g_param_p = 0;
			g_inter_p = 0;
			g_scratch_p = 0;
			
			g_mod = 0;
			g_val = 0;
			g_is_escape = 0;

			memset(g_params, 0, sizeof(g_params));
			memset(g_inters, 0, sizeof(g_inters));

			t_mach_push(TM_DETERMINE);
			break;

		case TM_READ:
			/* poll if required */
			if (wait) {
				struct pollfd fd = {
					.fd = STDIN_FILENO,
					.events = POLLIN | POLLPRI
				};
				poll(&fd, 1, -1); /* don't care about return value */
			}

			/* proceed to read */
			g_read_buf_len = read(STDIN_FILENO, g_read_buf, sizeof(g_read_buf));
			g_read_cursor = g_read_buf;
			g_read_end = g_read_buf + g_read_buf_len;
			break;

		/* terminating instructions */
		case TM_HALT_EMIT:
			if (g_is_escape) {
				uint32_t key_index = T_MIN(
					g_params[0], T_ARRAY_LENGTH(PC_KEYSPEC_TABLE)
				);
				uint32_t mod_index = T_MIN(
					g_params[1], T_ARRAY_LENGTH(PC_KEYMOD_TABLE)
				);
				if (g_val == '~') {
					/* F5+ keys with modifiers */
					g_val = PC_KEYSPEC_TABLE[key_index];
				}
				else if (0x50 <= g_val && g_val <= 0x54) {
					/* F1-F4 keys with modifiers */
					g_val -= 0x4f;
				}
				/* arrow keys are set so that g_val already has their code */
				g_mod = T_SPECIAL | PC_KEYMOD_TABLE[mod_index];
			}
			return T_POLL_CODE(g_mod, g_val);

		case TM_HALT_UNKNOWN:
			return T_EUNKNOWN;

		case TM_HALT_IF_EMPTY:
			if (!available) {
				return T_EEMPTY;
			}
		}
	}

	/* should never reach here */
	return T_ENOOPS;
}

/* +--- OUTPUT ----------------------------------------------------+ */
#ifdef TC_DEBUG_METRICS
void
t_debug_write_metrics_clear()
{
	g_debug_write_nflushed   = 0;
	g_debug_write_nstored    = 0;
	g_debug_write_f_nstored  = 0;
}

uint32_t
t_debug_write_nflushed()
{
	return g_debug_write_nflushed;
}

uint32_t
t_debug_write_nstored()
{
	return g_debug_write_nstored;
}

uint32_t
t_debug_write_f_nstored()
{
	return g_debug_write_f_nstored;
}
#endif /* TC_DEBUG_METRICS */

enum t_status
t_flush()
{
	enum t_status stat = T_OK;
	if (g_write_cursor > g_write_buf) {
		/* @TODO(max): more specific error than  T_EUNKNOWN? */
		uint32_t const limit = g_write_cursor - g_write_buf;
		stat = write(
			STDOUT_FILENO,
			g_write_buf,
			limit
		) < 0 ? T_EUNKNOWN : T_OK;

		if (stat >= 0) {
#ifdef TC_DEBUG_METRICS
			g_debug_write_nflushed += limit;
#endif
			g_write_cursor = g_write_buf;
		}
	}
	return stat;
}

enum t_status
t_write(
	uint8_t const *buffer,
	uint32_t length
) {
	if (!buffer) return T_ENULL;

	enum t_status stat = 0;
	while (stat >= 0 && length > 0) {
		uint32_t const limit = T_MIN(g_write_end - g_write_cursor, length);

		/* @SPEED(max): if buffer becomes large enough in the future,
		 * this memcpy might become expensive (for ex., if images are
		 * translated to another ansi sequence buffer that happens to
		 * be quite large.) */
		memcpy(g_write_cursor, buffer, limit);
		g_write_cursor += limit;
		buffer += limit;
		length -= limit;

#ifdef TC_DEBUG_METRICS
		g_debug_write_nstored += limit;
#endif

		if (g_write_cursor >= g_write_end) {
			stat = t_flush();
		}
	}
	return stat;
}

enum t_status
t_write_f(
	char const *format,
	...
) {
	if (!format) return T_ENULL;

	va_list vl;
	va_start(vl, format);

	int render_size = vsnprintf(
		g_write_f_buf,
		g_write_f_buf_size,
		format,
		vl
	);

	if (render_size < 0) {
		/* @TODO(max): more specific error code. */
		return T_EUNKNOWN;
	}

	if (render_size >= g_write_f_buf_size) {
		/* align for vectorized memcpy */
		uint32_t const target_size = T_ALIGN_UP(render_size, 16);
		if ((posix_memalign((void **) &g_write_f_buf, 16, target_size)) != 0) {
			/* TODO(max): EMALLOC or more specific error code? */
			return T_EMALLOC;
		}
		g_write_f_buf_size = target_size;

		va_start(vl, format);
		render_size = vsnprintf(
			g_write_f_buf,
			g_write_f_buf_size,
			format,
			vl
		);
	}
	va_end(vl);

#ifdef TC_DEBUG_METRICS
	g_debug_write_f_nstored += render_size;
#endif

	return t_write(
		(uint8_t const *) g_write_f_buf, 
		(uint32_t) render_size
	);
}

enum t_status
t_write_z(
	char const *string
) {
	if (!string) return T_ENULL;

	return t_write((uint8_t const *) string, strlen(string));
}
