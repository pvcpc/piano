#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include <stdint.h>
#include <string.h>

#include "t_util.h"
#include "t_base.h"

#define TM_TIMERS_MAX 16
#define TM_GLOBAL_READ_BUF_SIZE 256
#define TM_CODES_MAX 16
#define TM_SCRATCH_BUF_SIZE 256


enum t_poll_code
{
	/* parsing insructions */
	TM_DETERMINE,

	TM_NORMAL,
	TM_CONTROL,

	TM_ESCAPE_INIT,
	TM_ESCAPE,
	TM_ESCAPE_BRACKET_INIT,
	TM_ESCAPE_BRACKET,

	TM_SS3_FUNC_INIT,
	TM_SS3_FUNC,

	TM_PARAM,
	TM_PARAM_EMIT,

	TM_INTER,

	/* state change request instructions */
	TM_RESET,
	TM_READ,

	/* instructions that exit the machine while loop with return code */
	TM_HALT_UNKNOWN,
	TM_HALT_ON_EMPTY,
	TM_HALT_EMIT,
};

/* https://invisible-island.net/xterm/ctlseqs/ctlseqs.html#h3-PC-Style-Function-Keys */
/* @NOTE(max): the way T_* keys are assigned, this table is actually just
 * an identity table, so it may be removed if the modifiers are not
 * reassigned later. */
static enum t_event_mod const PC_KEYMOD_TABLE [] = {
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

static enum t_event_mod const PC_KEYFUNC_TABLE [] = {
	[11] =  1,
	[12] =  2,
	[13] =  3,
	[14] =  4,
	[15] =  5,
	[17] =  6,
	[18] =  7,
	[19] =  8,
	[20] =  9,
	[21] = 10,
	[23] = 11,
	[24] = 12,
};

/* all global variables are below and can be moved to a struct when necessary */
static struct termios         g_tios_old;

/* poll machine */
static uint8_t                g_read_buf     [TM_GLOBAL_READ_BUF_SIZE];
static uint8_t const         *g_cursor;
static uint8_t const         *g_end;
static int                    g_read_buf_len;

static enum t_poll_code       g_codes        [TM_CODES_MAX];
static uint32_t               g_code_p;

static uint8_t                g_scratch      [TM_SCRATCH_BUF_SIZE];
static uint32_t               g_scratch_p;

static struct t_event         g_ev;


static enum t_poll_code
t_mach_push(
	enum t_poll_code code
) {
	/* @TODO(max): overflow not protected, need proper error handling */
	return g_codes[g_code_p++] = code;
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
	/* @TODO(max): overflow not protected, need proper error handling */
	return g_ev.seq_buf[g_ev.seq_len++] = *g_cursor++;
}

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
}

void
t_cleanup()
{
	/* reset terminal settings back to normal */
	tcsetattr(STDIN_FILENO, TCSANOW, &g_tios_old);
}

char const *
t_status_string(
	enum t_status stat
) {
	switch (stat) {
	case T_EUNKNOWN:
		return "t_eunknown";
	case T_EEMPTY:
		return "t_eempty";
	case T_ENOOPS:
		return "t_enoops";
	case T_ENULL:
		return "t_enull";
	case T_OK:
		return "t_ok";
	default:
		return "undefined_status_code";
	}
}

enum t_status
t_event_clear(
	struct t_event *event
) {
	if (!event) return T_ENULL;

	memset(event, 0, sizeof(struct t_event));

	return T_OK;
}

enum t_status
t_poll(
	struct t_event *out
) {
	if (!out) return T_ENULL;

	t_mach_push(TM_RESET);

	while (g_code_p > 0) {
		enum t_poll_code const code      = t_mach_pop();
		uint32_t         const available = g_cursor <= g_end ? 
			g_end - g_cursor : 0;

		switch (code) {
		case TM_DETERMINE:
			if (!available) {
				t_mach_push(code);
				t_mach_push(TM_HALT_ON_EMPTY);
				t_mach_push(TM_READ);
			}
			else if (g_cursor[0] == '\x1b') {
				t_mach_push(TM_ESCAPE_INIT);
				t_mach_cursor_inc();
			}
			else if (0x00 <= g_cursor[0] &&
			                 g_cursor[0] <= 0x1f) {
				t_mach_push(TM_CONTROL);
			}
			else {
				t_mach_push(TM_NORMAL);
			}
			break;

		/* normal byte */
		case TM_NORMAL:
			t_mach_push(TM_HALT_EMIT);
			g_ev.val = t_mach_cursor_inc();
			break;

		/* control byte */
		case TM_CONTROL:
			t_mach_push(TM_HALT_EMIT);
			g_ev.mod |= T_CONTROL;
			g_ev.val = t_mach_cursor_inc() | 0x40;
			break;

		/* escape sequence parser */
		case TM_ESCAPE_INIT:
			t_mach_push(TM_ESCAPE);
			if (!available) {
				t_mach_push(TM_READ);
			}
			break;

		case TM_ESCAPE:
			g_ev.is_escape = 1;
			if (!available) {
				g_ev.val = '\x1b';
				t_mach_push(TM_HALT_EMIT);
			}
			else if (g_cursor[0] == '\x1b') {
				g_ev.val = '\x1b';
				t_mach_push(TM_HALT_EMIT);
				t_mach_cursor_inc();
			}
			else if (g_cursor[0] == '\x4f') {
				t_mach_push(TM_SS3_FUNC_INIT);
				t_mach_cursor_inc();
			}
			else if (g_cursor[0] == '[') {
				t_mach_push(TM_ESCAPE_BRACKET_INIT);
				t_mach_cursor_inc();
			}
			else {
				g_ev.mod |= T_ALT;
				g_ev.val = t_mach_cursor_inc();
				t_mach_push(TM_HALT_EMIT);
			}
			break;

		case TM_SS3_FUNC_INIT:
			t_mach_push(TM_SS3_FUNC);
			if (!available) {
				t_mach_push(TM_READ);
			}
			break;

		case TM_SS3_FUNC:
			if (!available) {
				g_ev.val = '\x4f';
				t_mach_push(TM_HALT_EMIT);
				t_mach_cursor_inc();
			}
			else {
				g_ev.mod |= T_FUNC;
				g_ev.val = t_mach_cursor_inc() - '\x4f';
				t_mach_push(TM_HALT_EMIT);
			}
			break;

		case TM_ESCAPE_BRACKET_INIT:
			t_mach_push(TM_ESCAPE_BRACKET);
			if (!available) {
				t_mach_push(TM_READ);
			}
			break;

		case TM_ESCAPE_BRACKET:
			if (!available) {
				g_ev.mod |= T_ALT;
				g_ev.val = '[';
				t_mach_push(TM_HALT_EMIT);
			}
			/* @NOTE(max): we can't just follow TM_PARAM -> TM_INTER 
			 * -> TM_NORMAL because TM_PARAM will emit a parameter 
			 * even if there are none present, so we explicitly check 
			 * for the required byte ranges. */
			else if (0x30 <= g_cursor[0] &&
			                 g_cursor[0] <= 0x3f) {
				t_mach_push(TM_PARAM);
			}
			else if (0x20 <= g_cursor[0] &&
			                 g_cursor[0] <= 0x2f) {
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
				t_mach_push(TM_HALT_ON_EMPTY);
				t_mach_push(TM_READ);
			}
			else if (g_cursor[0] < 0x30 ||
			                       0x3f < g_cursor[0]) {
				t_mach_push(TM_INTER);
				t_mach_push(TM_PARAM_EMIT);
			}
			else if (g_cursor[0] == ';') {
				t_mach_push(code);
				t_mach_push(TM_PARAM_EMIT);
				t_mach_cursor_inc();
			}
			else {
				t_mach_push(code);
				if (g_scratch_p < TM_SCRATCH_BUF_SIZE) {
					g_scratch[g_scratch_p++] = t_mach_cursor_inc();
				}
			}
			break;

		case TM_PARAM_EMIT:
			if (g_ev.n_params < T_PARAMS_MAX) {
				uint16_t result = 0;
				for (uint32_t i = 0; i < g_scratch_p; ++i) {
					result *= 10;
					result += g_scratch[i] - '0';
				}
				g_ev.params [g_ev.n_params++] = result;
				g_scratch_p = 0;
			}
			break;

		/* intermediate bytes */
		case TM_INTER:
			if (!available) {
				t_mach_push(code);
				t_mach_push(TM_HALT_ON_EMPTY);
				t_mach_push(TM_READ);
			}
			else if (g_cursor[0] < 0x20 ||
			                       0x2f < g_cursor[0]) {
				t_mach_push(TM_NORMAL);
			}
			else {
				t_mach_push(code);
				if (g_ev.n_inters < T_INTERS_MAX) {
					g_ev.inters[g_ev.n_inters++] = 
						t_mach_cursor_inc();
				}
			}
			break;

		/* interrupts */
		case TM_RESET:
			g_code_p = 0;
			g_scratch_p = 0;
			memset(&g_ev, 0, sizeof(struct t_event));
			t_mach_push(TM_DETERMINE);
			break;

		case TM_READ:
			g_read_buf_len = read(STDIN_FILENO, g_read_buf, sizeof(g_read_buf));
			g_cursor = g_read_buf;
			g_end = g_read_buf + g_read_buf_len;
			break;

		/* terminating instructions */
		case TM_HALT_EMIT:
			if (g_ev.val == '~') {
				/* F5+ keys with modifiers */
				uint32_t key_index = T_MIN(
					g_ev.params[0], T_ARRAY_LENGTH(PC_KEYFUNC_TABLE)
				);
				uint32_t mod_index = T_MIN(
					g_ev.params[1], T_ARRAY_LENGTH(PC_KEYMOD_TABLE)
				);
				g_ev.val = PC_KEYFUNC_TABLE[key_index];
				g_ev.mod = T_FUNC | PC_KEYMOD_TABLE[mod_index];
			}
			if (g_ev.is_escape && (0x50 <= g_ev.val && g_ev.val <= 0x54)) {
				/* F1-F4 keys with modifiers */
				uint32_t mod_index = T_MIN(
					g_ev.params[1], T_ARRAY_LENGTH(PC_KEYMOD_TABLE)
				);
				g_ev.val -= '\x4f';
				g_ev.mod = T_FUNC | PC_KEYMOD_TABLE[mod_index];
			}
			g_ev.qcode = T_QCODE(g_ev.mod, g_ev.val);
			memcpy(out, &g_ev, sizeof(struct t_event));
			return T_OK;

		case TM_HALT_UNKNOWN:
			return T_EUNKNOWN;

		case TM_HALT_ON_EMPTY:
			if (!available) {
				return T_EEMPTY;
			}
		}
	}

	/* should never reach here, but just in case */
	return T_ENOOPS;
}

enum t_status
t_write(
	uint8_t const *data,
	uint32_t length
) {
	if (!data) return T_ENULL;

	/* @TODO(max): sometime in the future get errno to report proper
	 * error, although, with stdout, that's not usually an issue. */
	return write(STDOUT_FILENO, data, length) < 0 ? T_EUNKNOWN : T_OK;
}

enum t_status
t_writez(
	char const *data
) {
	if (!data) return T_ENULL;

	return t_write((uint8_t const *) data, strlen(data));
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
