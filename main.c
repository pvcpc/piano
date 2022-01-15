#include <termios.h>
#include <unistd.h>
#include <poll.h>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>


enum input_stat
{
	IN_ENOOPS   = -5,
	IN_EUNKNOWN = -4,
	IN_EHALT    = -3,
	IN_ESETBUF  = -2,
	IN_ENULL    = -1,

	IN_OK       =  0,
};

char const *
in_stat_string(
	enum input_stat stat
) {
	switch (stat) {
	case IN_ENOOPS:
		return "IN_ENOOPS";
	case IN_EUNKNOWN:
		return "IN_EUNKNOWN";
	case IN_EHALT:
		return "IN_EHALT";
	case IN_ESETBUF:
		return "IN_ESETBUF";
	case IN_ENULL:
		return "IN_ENULL";
	case IN_OK:
		return "IN_OK";
	default:
		return "UNKNOWN_ERROR";
	}
}

enum input_mod
{
	IN_META    = 0x01,
	IN_CONTROL = 0x02,
	IN_ALT     = 0x04,
	IN_SHIFT   = 0x08,
};

#define IN_MAX_PARAMS 16
#define IN_MAX_INTERS 16

#define INM_MAX_OPCODES 16
#define INM_MAX_SCRATCH_BYTES 64

struct input 
{
	uint8_t mod; /* input modifiers (see above) */
	uint8_t params [IN_MAX_PARAMS]; /* parsed param integers if needed */
	uint8_t inters [IN_MAX_INTERS]; /* raw intermediate bytes if needed */
	uint8_t val; /* ascii character/escape sequence final byte */

	uint32_t n_params;
	uint32_t n_inters;
};

enum input_op
{
	INM_DETERMINE,

	INM_NORMAL,
	INM_CONTROL,
	INM_ESCAPE_INIT,
	INM_ESCAPE,
	INM_ESCAPE_BRACKET_INIT,
	INM_ESCAPE_BRACKET,

	INM_PARAM,
	INM_PARAM_EMIT,

	INM_INTER,

	INM_EMIT,

	INM_HALT_IF_EMPTY,
	INM_HALT_SETBUF,
	INM_HALT_UNKNOWN,
};

struct input_machine
{
	enum input_op opcodes [INM_MAX_OPCODES];
	uint8_t scratch [INM_MAX_SCRATCH_BYTES];

	uint32_t op_p;
	uint32_t scratch_p;

	struct input in;
	uint8_t const *cursor;
	uint8_t const *end;
};

enum input_op
inm_op_push(
	struct input_machine *mach,
	enum input_op op
) {
	return mach->opcodes[mach->op_p++] = op;
}


enum input_op
inm_op_pop(
	struct input_machine *mach
) {
	return mach->opcodes[--mach->op_p];
}

enum input_stat
in_reset(
	struct input_machine *mach
) {
	if (!mach) return IN_ENULL;

	mach->op_p = 0;
	mach->scratch_p = 0;
	memset(&mach->in, 0, sizeof(struct input));
	inm_op_push(mach, INM_DETERMINE);

	return IN_OK;
}

enum input_stat
in_init(
	struct input_machine *mach
) {
	if (!mach) return IN_ENULL;

	mach->cursor = NULL;
	mach->end = NULL;
	return in_reset(mach);
}

enum input_stat
in_setbuf(
	struct input_machine *mach,
	uint8_t const *buffer,
	uint32_t const length
) {
	if (!mach) return IN_ENULL;
	if (!buffer) return IN_ENULL;

	mach->cursor = buffer;
	mach->end = buffer + length;

	return IN_OK;
}

enum input_stat
in_next(
	struct input *out,
	struct input_machine *mach
) {
	if (!out) return IN_ENULL;
	if (!mach) return IN_ENULL;

	while (mach->op_p > 0) {
		/* useful calculations for each cycle */
		enum input_op const op = inm_op_pop(mach);
		uint32_t const available = mach->cursor <= mach->end ? 
			mach->end - mach->cursor : 0;

		/* */
		switch (op) {

		case INM_DETERMINE:
			if (!available) {
				inm_op_push(mach, op);
				inm_op_push(mach, INM_HALT_IF_EMPTY);
				inm_op_push(mach, INM_HALT_SETBUF);
			}
			else {
				if (mach->cursor[0] == '\x1b') {
					inm_op_push(mach, INM_ESCAPE_INIT);
					++mach->cursor;
				}
				else if (0x00 <= mach->cursor[0] &&
				                 mach->cursor[0] <= 0x1f) {
					inm_op_push(mach, INM_CONTROL);
				}
				else {
					inm_op_push(mach, INM_NORMAL);
				}
			}
			break;

		/* normal byte */
		case INM_NORMAL:
			mach->in.val = *mach->cursor++;
			inm_op_push(mach, INM_EMIT);
			break;

		/* control byte */
		case INM_CONTROL:
			mach->in.mod |= IN_CONTROL;
			mach->in.val = (*mach->cursor++) | 0x40;
			inm_op_push(mach, INM_EMIT);
			break;

		/* escape sequence parser */
		case INM_ESCAPE_INIT:
			inm_op_push(mach, INM_ESCAPE);
			if (!available) {
				inm_op_push(mach, INM_HALT_SETBUF);
			}
			break;

		case INM_ESCAPE:
			if (!available) {
				mach->in.val = '\x1b';
				inm_op_push(mach, INM_EMIT);
			}
			else if (mach->cursor[0] == '\x1b') {
				mach->in.val = '\x1b';
				inm_op_push(mach, INM_EMIT);
				++mach->cursor;
			}
			else if (mach->cursor[0] == '[') {
				inm_op_push(mach, INM_ESCAPE_BRACKET_INIT);
				++mach->cursor;
			}
			else {
				mach->in.mod |= IN_ALT;
				mach->in.val = *mach->cursor++;
				inm_op_push(mach, INM_EMIT);
			}
			break;

		case INM_ESCAPE_BRACKET_INIT:
			inm_op_push(mach, INM_ESCAPE_BRACKET);
			if (!available) {
				inm_op_push(mach, INM_HALT_SETBUF);
			}
			break;

		case INM_ESCAPE_BRACKET:
			if (!available) {
				mach->in.mod |= IN_ALT;
				mach->in.val = '[';
				inm_op_push(mach, INM_EMIT);
			}
			else if (0x30 <= mach->cursor[0] &&
			                 mach->cursor[0] <= 0x3f) {
				inm_op_push(mach, INM_PARAM);
			}
			else if (0x20 <= mach->cursor[0] &&
			                 mach->cursor[0] <= 0x2f) {
				inm_op_push(mach, INM_INTER);
			}
			else {
				inm_op_push(mach, INM_NORMAL);
			}
			break;

		/* parameter parser */
		case INM_PARAM:
			if (!available) {
				inm_op_push(mach, op);
				inm_op_push(mach, INM_HALT_IF_EMPTY);
				inm_op_push(mach, INM_HALT_SETBUF);
			}
			else if (mach->cursor[0] < 0x30 ||
			                           0x3f < mach->cursor[0]) {
				inm_op_push(mach, INM_INTER);
				inm_op_push(mach, INM_PARAM_EMIT);
			}
			else if (mach->cursor[0] == ';') {
				inm_op_push(mach, op);
				inm_op_push(mach, INM_PARAM_EMIT);
				++mach->cursor;
			}
			else {
				inm_op_push(mach, op);
				if (mach->scratch_p < INM_MAX_SCRATCH_BYTES) {
					mach->scratch[mach->scratch_p++] = *mach->cursor++;
				}
			}
			break;

		case INM_PARAM_EMIT:
			if (mach->in.n_params < IN_MAX_PARAMS) {
				uint8_t result = 0;
				for (uint32_t i = 0; i < mach->scratch_p; ++i) {
					result *= 10;
					result += mach->scratch[i] - '0';
				}
				mach->in.params [mach->in.n_params++] = result;
				mach->scratch_p = 0;
			}
			break;

		/* intermediate bytes */
		case INM_INTER:
			if (!available) {
				inm_op_push(mach, op);
				inm_op_push(mach, INM_HALT_IF_EMPTY);
				inm_op_push(mach, INM_HALT_SETBUF);
			}
			else if (mach->cursor[0] < 0x20 ||
			                           0x2f < mach->cursor[0]) {
				inm_op_push(mach, INM_NORMAL);
			}
			else {
				inm_op_push(mach, op);
				if (mach->in.n_inters < IN_MAX_INTERS) {
					mach->in.inters[mach->in.n_inters++] = *mach->cursor++;
				}
			}
			break;

		/* terminating instructions */
		case INM_EMIT:
			memcpy(out, &mach->in, sizeof(struct input));
			in_reset(mach);
			return IN_OK;

		case INM_HALT_UNKNOWN:
			return IN_EUNKNOWN;

		case INM_HALT_SETBUF:
			return IN_ESETBUF;

		case INM_HALT_IF_EMPTY:
			if (!available) {
				return IN_EHALT;
			}
		}
	}
	return IN_ENOOPS;
}

int
main(void)
{
	/* disable "canonical mode" so we can read input as it's typed. We
	 * get two copies so we can restore the previous settings at he end 
	 * of the program.
	 */
	struct termios old, now;
	tcgetattr(STDIN_FILENO, &old);
	tcgetattr(STDIN_FILENO, &now);

	now.c_lflag &= ~(ICANON | ECHO);
	now.c_cc[VTIME] = 0; /* min of 0 deciseconds for read(3) to block */
	now.c_cc[VMIN] = 0; /* min of 0 characters for read(3) */
	tcsetattr(STDIN_FILENO, TCSANOW, &now);

	/* test out ansi commands; clear screen */
	unsigned char const cmd_clear[] = "\x1b[2J";
	write(STDOUT_FILENO, cmd_clear, sizeof(cmd_clear));

	/* move cursor to top left */
	unsigned char const cmd_mov_topleft[] = "\x1b[1;1H";
	write(STDOUT_FILENO, cmd_mov_topleft, sizeof(cmd_mov_topleft));

	/* request "device status report," i.e., current cursor position. */
	unsigned char const cmd_device_report[] = "\x1b[6n";
	write(STDOUT_FILENO, cmd_device_report, sizeof(cmd_device_report));

	/* setup input facility */
	struct input_machine machine;
	in_init(&machine);

	uint8_t buf [256];
	int len;

	while (1) {

		struct input in;
		enum input_stat stat;

		while ((stat = in_next(&in, &machine)) == IN_ESETBUF) {
			len = read(STDIN_FILENO, buf, sizeof(buf));
			in_setbuf(&machine, buf, len);
		}

		if (stat < 0  && stat != IN_EHALT) {
			fprintf(stderr, "in_next failed with error %s\n",
				in_stat_string(stat));
		}
		else if (stat >= 0) {
			printf("cmd: ");
			for (uint32_t i = 0; i < len; ++i) {
				printf("%02x ", buf[i]);
			}
			printf("mod: %02x val: %02x\n", in.mod, in.val);
			if (in.n_params) {
				puts("\tparams:");
				for (uint32_t i = 0; i < in.n_params; ++i) {
					printf("\t%2d: %d\n", i, in.params[i]);
				}
			}
			if (in.n_inters) {
				puts("\tinters:");
				for (uint32_t i = 0; i < in.n_inters; ++i) {
					printf("\t%2d: %02x\n", i, in.inters[i]);
				}
			}
		}
	}

	/* reset terminal settings back to normal */
	tcsetattr(STDIN_FILENO, TCSANOW, &old);
	return 0;
}
