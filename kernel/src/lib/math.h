#ifndef _DICRON_LIB_MATH_H
#define _DICRON_LIB_MATH_H

#include <stddef.h>
#include <stdint.h>

/* Alignment (Must be a power of 2) */
static inline uint64_t math_align_up(uint64_t val, uint64_t align)
{
	return (val + align - 1) & ~(align - 1);
}

static inline uint64_t math_align_down(uint64_t val, uint64_t align)
{
	return val & ~(align - 1);
}

/* Page boundaries (Special case of alignment) */
static inline uint64_t math_page_align_up(uint64_t val)
{
	return math_align_up(val, 4096);
}

static inline uint64_t math_page_align_down(uint64_t val)
{
	return math_align_down(val, 4096);
}

/* Division rounding to ceiling (useful for block counts) */
static inline uint64_t math_div_ceil(uint64_t num, uint64_t den)
{
	if (den == 0) return 0;
	return (num + den - 1) / den;
}

/* Standard Min/Max */
static inline int64_t math_max(int64_t a, int64_t b)
{
	return (a > b) ? a : b;
}

static inline int64_t math_min(int64_t a, int64_t b)
{
	return (a < b) ? a : b;
}

static inline uint64_t math_max_u(uint64_t a, uint64_t b)
{
	return (a > b) ? a : b;
}

static inline uint64_t math_min_u(uint64_t a, uint64_t b)
{
	return (a < b) ? a : b;
}

/* Absolute value */
static inline int64_t math_abs(int64_t a)
{
	return (a < 0) ? -a : a;
}

/* Buddy order / Log2 arithmetic */
static inline unsigned int math_log2_up(uint64_t n)
{
	if (n <= 1) return 0;
	uint64_t val = n - 1;
	unsigned int order = 0;
	while (val > 0) {
		val >>= 1;
		order++;
	}
	return order;
}

#endif
