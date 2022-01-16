#ifndef INCLUDE_T_BASE_H
#define INCLUDE_T_BASE_H

#define T_PARAMS_MAX 16
#define T_INTERS_MAX 16
#define T_SEQUENCE_BUF_SIZE 16

enum t_status
{
	T_EUNKNOWN = -4,
	T_EEMPTY   = -3,
	T_ENOOPS   = -2,
	T_ENULL    = -1,
	T_OK       =  0,
};

enum t_event_mod
{
	T_SHIFT   = 0x01,
	T_ALT     = 0x02,
	T_CONTROL = 0x04,
	T_META    = 0x08,
	T_FUNC    = 0x10,
	T_TIMER   = 0x20,
};

struct t_event
{
	uint8_t  mod;
	uint8_t  val;
	uint16_t qcode; /* see T_QCODE macro below */

	uint16_t params [T_PARAMS_MAX];
	uint8_t  inters [T_INTERS_MAX];
	uint32_t n_params;
	uint32_t n_inters;

	uint8_t  seq_buf [T_SEQUENCE_BUF_SIZE];
	uint32_t seq_len;

	uint8_t  is_escape; /* 1 if an escape sequence, 0 otherwise */
};

/* bitshift-or together mod/val values for easy switch/case or table lookups */
#define T_QCODE(Mod, Val) (((Mod) << 8) | (Val))

/* common rendition codes */
#define TR_RESET      "\x1b[0m"

#define TR_FG_BLACK   "\x1b[30m"
#define TR_FG_RED     "\x1b[31m"
#define TR_FG_GREEN   "\x1b[32m"
#define TR_FG_YELLOW  "\x1b[33m"
#define TR_FG_BLUE    "\x1b[34m"
#define TR_FG_MAGENTA "\x1b[35m"
#define TR_FG_CYAN    "\x1b[36m"
#define TR_FG_WHITE   "\x1b[37m"

#define TR_BG_BLACK   "\x1b[40m"
#define TR_BG_RED     "\x1b[41m"
#define TR_BG_GREEN   "\x1b[42m"
#define TR_BG_YELLOW  "\x1b[43m"
#define TR_BG_BLUE    "\x1b[44m"
#define TR_BG_MAGENTA "\x1b[45m"
#define TR_BG_CYAN    "\x1b[46m"
#define TR_BG_WHITE   "\x1b[47m"


void
t_setup();

void
t_cleanup();

char const *
t_status_string(
	enum t_status stat
);

enum t_status
t_event_clear(
	struct t_event *event
);

enum t_status
t_poll(
	struct t_event *out
);

enum t_status
t_write(
	uint8_t const *data,
	uint32_t length
);

enum t_status
t_writez(
	char const *data
);

#endif /* INCLUDE_T_BASE_H */
