#ifndef INCLUDE_T_UTIL_H
#define INCLUDE_T_UTIL_H

#include <stdint.h>

#define T_MAX(a, b) ((a) >= (b) ? (a) : (b))
#define T_MIN(a, b) ((a) <= (b) ? (a) : (b))
#define T_CLAMP(v, lo, hi) T_MIN(T_MAX(v, lo), hi)
#define T_ABS(v) ((v) < 0 ? -(v) : (v))

#define T_ALIGN_UP(v, boundary) (((v + boundary - 1) / boundary) * boundary)

#define T_ARRAY_LENGTH(arr) (sizeof(arr) / sizeof(*arr))

#define T_KILO(N) (N * (1 << 10))
#define T_MEGA(N) (N * (1 << 10))
#define T_GIGA(N) (N * (1 << 10))


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
		return "t_enull";
	case T_EPARAM:
		return "T_EPARAM";
	case T_EMALLOC:
		return "T_EMALLOC";
	
	case T_EUNKNOWN:
		return "t_eunknown";
	case T_EEMPTY:
		return "t_eempty";
	case T_ENOOPS:
		return "t_enoops";
	
	/* info */
	case T_OK:
		return "t_ok";
	default:
		return "undefined_status_code";
	}
}


#endif /* INCLUDE_T_UTIL_H */
