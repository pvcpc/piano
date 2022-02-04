#ifndef INCLUDE_T_BASE_H
#define INCLUDE_T_BASE_H

#include "t_util.h"

/* @TUNABLE compile-time constants (see README):
 * - TC_GLOBAL_READ_BUFFER_SIZE (default 256): 
 *   Set the user-space read(3) buffer size. It does not need to be
 *   large as `t_setup()` will configure read to be nonblocking, and
 *   only snippets of user input are expected.
 *
 * - TC_GLOBAL_WRITE_BUFFER_SIZE (default 65536):
 *   Set the user-space write(3) buffer size. It may be desirable to
 *   have a large write buffer size, especially to push through large
 *   sequences through the terminal to render images, for instance.
 */
#ifndef TC_GLOBAL_READ_BUFFER_SIZE
#  define TC_GLOBAL_READ_BUFFER_SIZE 256
#endif

#ifndef TC_GLOBAL_WRITE_BUFFER_SIZE
#  define TC_GLOBAL_WRITE_BUFFER_SIZE (1u << 16) /* 64K default buffer */
#endif

/* +--- GENERAL ---------------------------------------------------+ */
void
t_setup();

void
t_cleanup();

double
t_elapsed();

double
t_sleep(
	double seconds
);

enum t_status
t_termsize(
	int32_t *out_width,
	int32_t *out_height
);

/* +--- INPUT -----------------------------------------------------+ */
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

/* +--- OUTPUT ----------------------------------------------------+ */
enum t_status
t_flush();

enum t_status
t_write(
	uint8_t const *buffer,
	uint32_t const length
);

enum t_status
t_write_f(
	char const *format,
	...
);

enum t_status
t_write_z(
	char const *string
);

/* +--- DEBUG ----------------------------------------------------+ */
#ifdef TC_DEBUG_METRICS
void
t_debug_write_metrics_clear();

uint32_t
t_debug_write_nflushed();

uint32_t
t_debug_write_nstored();

uint32_t
t_debug_write_f_nstored();
#else
#  define t_debug_write_metrics_clear()
#  define t_debug_write_nstored() 0
#  define t_debug_write_nflushed() 0
#  define t_debug_write_p_nstored() 0
#endif

#endif /* INCLUDE_T_BASE_H */
