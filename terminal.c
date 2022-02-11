#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include <termios.h>
#include <unistd.h>

#include "terminal.h"

/* @TUNABLE T_READ_BUFSZ */
#ifndef T_READ_BUFSZ
#  define T_READ_BUFSZ 256
#endif

/* @TUNABLE T_SCRATCH_BUFSZ */
#ifndef T_SCRATCH_BUFSZ
#  define T_SCRATCH_BUFSZ 256
#endif

/* @TUNABLE T_WRITE_BUFSZ */
#ifndef T_WRITE_BUFSZ
#  define T_WRITE_BUFSZ 65536
#endif

#define T__POLL_CODES_MAX 16
#define T__PARAMS_MAX 4
#define T__INTERS_MAX 4

/* @GLOBAL */
static struct termios         g_tios_old;

/* input */
static u8                     g_read_buf     [T_READ_BUFSZ];
static u8 const              *g_read_cursor;
static u8 const              *g_read_end;
static u32                    g_read_buf_len;

/* output */
static u8                     g_write_buf    [T_WRITE_BUFSZ];
static u8                    *g_write_cursor = g_write_buf;
static u8 const * const       g_write_end    = g_write_buf + T_WRITE_BUFSZ;

static char                  *g_write_f_buf;
static u32                    g_write_f_buf_size;

bool
t_manager_setup()
{
	if (!isatty(STDIN_FILENO)) {
		return false;
	}

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

	return true;
}


void
t_manager_cleanup()
{
	/* flush any remaining output in case things like `cursor_show()`
	 * are called at the end to restore defaults.
	 */
	t_flush();

	/* reset terminal settings back to normal */
	tcsetattr(STDIN_FILENO, TCSANOW, &g_tios_old);
}

enum epm_code /* escape pars(er|ing) machine */
{
	/* parsing codes */
	EPM_DETERMINE,

	EPM_NORMAL,
	EPM_CONTROL,

	EPM_ESCAPE,
	EPM_ESCAPE_BRACKET,

	EPM_SS3_FUNC,

	EPM_PARAM,
	EPM_PARAM_EMIT,

	EPM_INTER,

	/* state change codes */
	EPM_RESET,
	EPM_READ,

	/* halt codes */
	EPM_HALT_UNKNOWN,
	EPM_HALT_IF_EMPTY,
	EPM_HALT_EMIT,
};

/* @GLOBAL */
/* https://invisible-island.net/xterm/ctlseqs/ctlseqs.html#h3-PC-Style-Function-Keys */
/* @NOTE(max): the way T_* keys are assigned, this table is actually just
 * an identity table, so it may be removed if the modifiers are not
 * reassigned later. */
static enum t_poll_modifier const g_pc_keymod_table [] = {
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

static enum t_poll_special const g_pc_keyspec_table [] = {
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
static enum epm_code          g_codes        [T__POLL_CODES_MAX];
static u32                    g_code_p;

static u16                    g_params       [T__PARAMS_MAX];
static u32                    g_param_p;

static u8                     g_inters       [T__INTERS_MAX];
static u32                    g_inter_p;

static u8                     g_scratch      [T_SCRATCH_BUFSZ];
static u32                    g_scratch_p;

static u8                     g_mod;
static u8                     g_val;
static bool                   g_is_escape;
           
static enum epm_code
epm_push(
	enum epm_code code
) {
	/* @TODO(max): overflow not protected, need proper error handling */
	return g_codes[g_code_p++] = code;
}

static enum epm_code
epm_push_init(
	enum epm_code code
) {
	/* @NOTE(max): this was a common idiom in the poll epmine below,
	 * and most codes using it were postfixed with "_INIT," hence the
	 * function name.
	 *
	 * @TODO(max): this function may be confusing when writing out
	 * case statements in the actual epmine since there is no 
	 * indication that a code depends on this type of form.
	 */
	epm_push(code);
	if (g_read_end <= g_read_cursor) {
		epm_push(EPM_READ);
	}
	return code;
}

enum epm_code
epm_pop()
{
	/* @TODO(max): underflow not protected, need proper error handling */
	return g_codes[--g_code_p];
}

static u8
epm_cursor_inc() 
{
	/* @TODO(max): overflow not protected, bad? */
	return *g_read_cursor++;
}

u32
t_poll()
{
	if (!g_code_p) epm_push(EPM_RESET);

	while (g_code_p > 0) {
		enum epm_code const code = epm_pop();
		u32 const available = g_read_cursor <= g_read_end ? 
			g_read_end - g_read_cursor : 0;

		switch (code) {
		case EPM_DETERMINE:
			if (!available) {
				epm_push(code);
				epm_push(EPM_HALT_IF_EMPTY);
				epm_push(EPM_READ);
			}
			else if (g_read_cursor[0] == '\x1b') {
				epm_push_init(EPM_ESCAPE);
				epm_cursor_inc();
			}
			else if (0x00 <= g_read_cursor[0] &&
			                 g_read_cursor[0] <= 0x1f) {
				epm_push(EPM_CONTROL);
			}
			else {
				epm_push(EPM_NORMAL);
			}
			break;

		case EPM_NORMAL:
			epm_push(EPM_HALT_EMIT);
			g_val = epm_cursor_inc();
			break;

		case EPM_CONTROL:
			epm_push(EPM_HALT_EMIT);
			g_mod |= T_CONTROL;
			g_val = epm_cursor_inc() | 0x40;
			break;

		/* escape sequence parser */
		case EPM_ESCAPE:
			g_is_escape = 1;
			if (!available) {
				g_val = '\x1b';
				epm_push(EPM_HALT_EMIT);
			}
			else if (g_read_cursor[0] == '\x1b') {
				g_val = '\x1b';
				epm_push(EPM_HALT_EMIT);
				epm_cursor_inc();
			}
			else if (g_read_cursor[0] == '\x4f') {
				epm_push_init(EPM_SS3_FUNC);
				epm_cursor_inc();
			}
			else if (g_read_cursor[0] == '[') {
				epm_push_init(EPM_ESCAPE_BRACKET);
				epm_cursor_inc();
			}
			else {
				g_mod |= T_ALT;
				g_val = epm_cursor_inc();
				epm_push(EPM_HALT_EMIT);
			}
			break;

		case EPM_SS3_FUNC:
			if (!available) {
				g_val = '\x4f';
				epm_push(EPM_HALT_EMIT);
				epm_cursor_inc();
			}
			else {
				g_mod |= T_SPECIAL;
				g_val = epm_cursor_inc() - '\x4f';
				epm_push(EPM_HALT_EMIT);
			}
			break;

		case EPM_ESCAPE_BRACKET:
			if (!available) {
				g_mod |= T_ALT;
				g_val = '[';
				epm_push(EPM_HALT_EMIT);
			}
			/* @NOTE(max): we can't just follow EPM_PARAM -> M_INTER 
			 * -> EPM_NORMAL because M_PARAM will emit a parameter 
			 * even if there are none present, so we explicitly check 
			 * for the required byte ranges. */
			else if (0x30 <= g_read_cursor[0] &&
			                 g_read_cursor[0] <= 0x3f) {
				epm_push(EPM_PARAM);
			}
			else if (0x20 <= g_read_cursor[0] &&
			                 g_read_cursor[0] <= 0x2f) {
				epm_push(EPM_INTER);
			}
			else {
				epm_push(EPM_NORMAL);
			}
			break;

		/* parameter parser */
		case EPM_PARAM:
			if (!available) {
				epm_push(code);
				epm_push(EPM_HALT_IF_EMPTY);
				epm_push(EPM_READ);
			}
			else if (g_read_cursor[0] < 0x30 ||
			                       0x3f < g_read_cursor[0]) {
				epm_push(EPM_INTER);
				epm_push(EPM_PARAM_EMIT);
			}
			else if (g_read_cursor[0] == ';') {
				epm_push(code);
				epm_push(EPM_PARAM_EMIT);
				epm_cursor_inc();
			}
			else {
				epm_push(code);
				if (g_scratch_p < T_SCRATCH_BUFSZ) {
					g_scratch[g_scratch_p++] = epm_cursor_inc();
				}
			}
			break;

		case EPM_PARAM_EMIT:
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
		case EPM_INTER:
			if (!available) {
				epm_push(code);
				epm_push(EPM_HALT_IF_EMPTY);
				epm_push(EPM_READ);
			}
			else if (g_read_cursor[0] < 0x20 ||
			                       0x2f < g_read_cursor[0]) {
				epm_push(EPM_NORMAL);
			}
			else {
				epm_push(code);
				if (g_inter_p < T__INTERS_MAX) {
					g_inters[g_inter_p++] = epm_cursor_inc();
				}
			}
			break;

		/* state updates */
		case EPM_RESET:
			g_code_p = 0;
			g_param_p = 0;
			g_inter_p = 0;
			g_scratch_p = 0;
			
			g_mod = 0;
			g_val = 0;
			g_is_escape = 0;

			memset(g_params, 0, sizeof(g_params));
			memset(g_inters, 0, sizeof(g_inters));

			epm_push(EPM_DETERMINE);
			break;

		case EPM_READ:
			g_read_buf_len = read(STDIN_FILENO, g_read_buf, sizeof(g_read_buf));
			g_read_cursor = g_read_buf;
			g_read_end = g_read_buf + g_read_buf_len;
			break;

		/* terminating instructions */
		case EPM_HALT_EMIT:
			if (g_is_escape) {
				uint32_t key_index = MIN(
					g_params[0], ARRAY_LENGTH(g_pc_keyspec_table)
				);
				uint32_t mod_index = MIN(
					g_params[1], ARRAY_LENGTH(g_pc_keymod_table)
				);
				if (g_val == '~') {
					/* F5+ keys with modifiers */
					g_val = g_pc_keyspec_table[key_index];
				}
				else if (0x50 <= g_val && g_val <= 0x54) {
					/* F1-F4 keys with modifiers */
					g_val -= 0x4f;
				}
				/* arrow keys are set so that g_val already has their code */
				g_mod = T_SPECIAL | g_pc_keymod_table[mod_index];
			}
			return T_POLL_CODE(g_mod, g_val);

		case EPM_HALT_UNKNOWN:
			return T_POLL_CODE(T_ERROR, T_UNKNOWN);

		case EPM_HALT_IF_EMPTY:
			if (!available) {
				return T_POLL_CODE(T_ERROR, T_DISCARD);
			}
		}
	}

	/* @TODO(max): should not happen, log? */
	return 0;
}

u32
t_flush()
{
	u32 const limit = g_write_cursor - g_write_buf;

	if (limit && write(STDOUT_FILENO, g_write_buf, limit) < 0) {
		/* @TODO(max): log or something? who close the damn stream */
		return 0;
	}

	g_write_cursor = g_write_buf;
	return limit;
}

u32
t_write(u8 const *message, u32 length) 
{
	if (!message) return 0;

	u32 const original_length = length;

	while (length > 0) {
		u32 const limit = MIN(g_write_end - g_write_cursor, length);

		memcpy(g_write_cursor, message, limit);
		g_write_cursor += limit;
		message += limit;
		length -= limit;

		if (g_write_cursor >= g_write_end) {
			if (!t_flush()) goto e_flush;
		}
	}

e_flush:
	return original_length - length;
}

u32
t_write_f(char const *format, ...) 
{
	if (!format) return 0;

	va_list vl;
	va_start(vl, format);

	int render_size = vsnprintf(
		g_write_f_buf,
		g_write_f_buf_size,
		format,
		vl
	);

	if (render_size < 0) {
		/* @TODO(max): log this once logging is implemented. */
		return 0;
	}

	if (render_size >= g_write_f_buf_size) {
		/* align for vectorized memcpy */
		uint32_t const target_size = ALIGN_UP(render_size, 16);
		if ((posix_memalign((void **) &g_write_f_buf, 16, target_size)) != 0) {
			/* TODO(max): log this once logging is impleemnted */
			return 0;
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

	return t_write((u8 const *) g_write_f_buf, (u32) render_size);
}

u32
t_writez(char const *string) 
{
	if (!string) return 0;

	return t_write((u8 const *) string, strlen(string));
}
