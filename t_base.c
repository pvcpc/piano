#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "t_base.h"

#define T__POLL_CODES_MAX 16
#define T__PARAMS_MAX 4
#define T__INTERS_MAX 4
#define T__SCRATCH_BUFFER_SIZE 256


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

/* OUTPUT */
/* @SPEED(max): check g_write_buf alignment for vectorized memcpy */
static uint8_t                g_write_buf    [TC_GLOBAL_WRITE_BUFFER_SIZE];
static uint8_t               *g_write_cursor = g_write_buf;
static uint8_t const * const  g_write_end    = g_write_buf + TC_GLOBAL_WRITE_BUFFER_SIZE;


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


/* INPUT */
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


/* output */
enum t_status
t_flush()
{
	enum t_status stat = T_OK;
	if (g_write_cursor > g_write_buf) {
		/* @TODO(max): more specific error than  T_EUNKNOWN? */
		stat = write(
			STDOUT_FILENO,
			g_write_buf,
			g_write_cursor - g_write_buf
		) < 0 ? T_EUNKNOWN : T_OK;

		if (stat >= 0) {
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

		if (g_write_cursor >= g_write_buf) {
			stat = t_flush();
		}
	}
	return stat;
}

enum t_status
t_write_z(
	char const *string
) {
	if (!string) return T_ENULL;

	return t_write((uint8_t const *) string, strlen(string));
}

enum t_status
t_write_p(
	uint8_t const *param_sequence,
	...
) {
	if (!param_sequence) return T_ENULL;

	/* @SPEED(max): check alignment for vectorized memcpy */
	uint8_t tmp [32];
	uint32_t tmp_p = 0;

	uint8_t dec [16]; /* decimal conversion buffer */
	uint32_t dec_p = 0;

	va_list vl;
	va_start(vl, param_sequence);

	while (*param_sequence) {
		if (*param_sequence == '%') {
			/* render parameter as decimal */
			int arg = va_arg(vl, int) & 0xffff;
			while (arg > 0) {
				dec[dec_p++] = (arg % 10) + '0';
				arg /= 10;
			}
			while (dec_p) {
				tmp [tmp_p++] = dec[--dec_p];
			}
		}
		else {
			tmp[tmp_p++] = *param_sequence;
		}
		++param_sequence;
	}
	va_end(vl);

	return t_write(tmp, tmp_p);
}
