#ifndef INCLUDE_T_UTIL_H
#define INCLUDE_T_UTIL_H

#include <stdbool.h>
#include <stdint.h>

/* compile-time constants (see README for details):
 * - TC_DEBUG_METRICS (default <undefined>): 
 *   Enable debug metrics where possible.
 */

#define T_MAX(a, b) ((a) >= (b) ? (a) : (b))
#define T_MIN(a, b) ((a) <= (b) ? (a) : (b))
#define T_CLAMP(v, lo, hi) T_MIN(T_MAX(v, lo), hi)
#define T_ABS(v) ((v) < 0 ? -(v) : (v))

#define T_ALIGN_UP(v, boundary) (((v + boundary - 1) / boundary) * boundary)

#define T_KILO(N) (N * (1 << 10))
#define T_MEGA(N) (N * (1 << 10))
#define T_GIGA(N) (N * (1 << 10))

#define T_ARRAY_LENGTH(arr) (sizeof(arr) / sizeof(*arr))


enum t_status
{
	/* err (< 0) */
	T_ENULL          = -65536,
	T_EPARAM,
	T_EMALLOC,

	T_EUNKNOWN,
	T_EEMPTY,
	T_ENOOPS,

	/* info (>= 0) */
	T_OK             =  0,
};

static inline char const *
t_status_string(
	enum t_status stat
) {
	switch (stat) {
	/* error */
	case T_ENULL:
		return "T_ENULL";
	case T_EPARAM:
		return "T_EPARAM";
	case T_EMALLOC:
		return "T_EMALLOC";
	
	case T_EUNKNOWN:
		return "T_EUNKNOWN";
	case T_EEMPTY:
		return "T_EEMPTY";
	case T_ENOOPS:
		return "T_ENOOPS";
	
	/* info */
	case T_OK:
		return "T_OK";
	default:
		return "UNDEFINED_STATUS_CODE";
	}
}


#endif /* INCLUDE_T_UTIL_H */
