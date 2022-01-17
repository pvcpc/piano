#ifndef INCLUDE_T_BASE_H
#define INCLUDE_T_BASE_H

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

void
t_setup();

void
t_cleanup();

double
t_elapsed();

double
t_delta();

/* input */
enum t_status
t_event_clear(
	struct t_event *event
);

enum t_status
t_poll(
	struct t_event *out
);

/* output */
enum t_status
t_flush();

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