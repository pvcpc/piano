#ifndef INCLUDE_T_UTIL_H
#define INCLUDE_T_UTIL_H

#define T_MAX(a, b) ((a) >= (b) ? (a) : (b))
#define T_MIN(a, b) ((a) <= (b) ? (a) : (b))
#define T_CLAMP(v, lo, hi) T_MIN(T_MAX(v, lo), hi)

#define T_ARRAY_LENGTH(arr) (sizeof(arr) / sizeof(*arr))


enum t_status
{
	T_EUNKNOWN = -4,
	T_EEMPTY   = -3,
	T_ENOOPS   = -2,
	T_ENULL    = -1,
	T_OK       =  0,
};


#endif /* INCLUDE_T_UTIL_H */
