#ifndef INCLUDE_T_BASE_H
#define INCLUDE_T_BASE_H

#include "t_util.h"

/* tunable compile-time constants */
#ifndef TC_GLOBAL_READ_BUFFER_SIZE
#  define TC_GLOBAL_READ_BUFFER_SIZE 256
#endif

#ifndef TC_GLOBAL_WRITE_BUFFER_SIZE
#  define TC_GLOBAL_WRITE_BUFFER_SIZE (1u << 16) /* 64K default buffer */
#endif


void
t_setup();

void
t_cleanup();

double
t_elapsed();

enum t_status
t_termsize(
	int32_t *out_width,
	int32_t *out_height
);

/* INPUT */
enum t_poll_mod
{
	T_SHIFT      = 0x01,
	T_ALT        = 0x02,
	T_CONTROL    = 0x04,
	T_META       = 0x08,
	T_SPECIAL    = 0x10, /* see below */
	T_ERROR      = 0x80,
};

enum t_poll_special
{
	T_F0         = 0,
	T_F1,
	T_F2,
	T_F3,
	T_F4,
	T_F5,
	T_F6,
	T_F7,
	T_F8,
	T_F9,
	T_F10,
	T_F11,
	T_F12,

	T_UP         = 65,
	T_DOWN,
	T_RIGHT,
	T_LEFT,
};

#define T_POLL_CODE(Mod, Val) (((Mod) << 8) | (Val))

int32_t
t_poll();


/* OUTPUT */
enum t_status
t_flush();

enum t_status
t_write(
	uint8_t const *buffer,
	uint32_t const length
);

enum t_status
t_write_z(
	char const *string
);

enum t_status
t_write_p(
	uint8_t const *param_sequence,
	...
);

#endif /* INCLUDE_T_BASE_H */
