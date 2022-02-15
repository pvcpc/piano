#ifndef INCLUDE__COMMON_H
#define INCLUDE__COMMON_H

#include <stdint.h>
#include <stdbool.h>

/* I don't know why integers are so verbose when they're used everywhere */
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

union any_int
{
	s8   _s8;
	s16  _s16;
	s32  _s32;

	u8   _u8;
	u16  _u16;
	u32  _u32;
};

#define MAX(a, b) ((a) >= (b) ? (a) : (b))
#define MIN(a, b) ((a) <= (b) ? (a) : (b))
#define CLAMP(v, lo, hi) T_MIN(T_MAX(v, lo), hi)
#define ABS(v) ((v) < 0 ? -(v) : (v))

#define ALIGN_UP(v, bnd) ((((v) + (bnd) - 1) / (bnd)) * (bnd))

#define ARRAY_LENGTH(arr) (sizeof(arr) / sizeof(*arr))


#endif /* INCLUDE__COMMON_H */
