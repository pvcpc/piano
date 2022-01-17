#ifndef INCLUDE_T_INPUT_H
#define INCLUDE_T_INPUT_H

#include "t_util.h"

#define T_PARAMS_MAX 4
#define T_INTERS_MAX 4
#define T_SEQUENCE_BUF_SIZE 16


enum t_event_mod
{
	T_SHIFT   = 0x01,
	T_ALT     = 0x02,
	T_CONTROL = 0x04,
	T_META    = 0x08,
	T_FUNC    = 0x10,
	T_TIMER   = 0x20, /* @TODO(max): not implemented */
};

struct t_event
{
	uint8_t  mod;
	uint8_t  val;
	double   delta; /* for T_TIMER only, time elapsed since add in seconds */
	double   elapsed; /* for T_TIMER only, time between last evocation in seconds */
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

enum t_status
t_event_clear(
	struct t_event *event
);

enum t_status
t_poll(
	struct t_event *out
);

#endif /* INCLUDE_T_INPUT_H */
