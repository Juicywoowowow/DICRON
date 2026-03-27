#ifndef _DICRON_TIME_H
#define _DICRON_TIME_H

#include <stdint.h>

/* Returns the number of milliseconds since boot. */
uint64_t ktime_ms(void);

/* Blocks the current execution for `ms` milliseconds. */
void ksleep_ms(uint32_t ms);

/* Returns the number of nanoseconds since boot. */
uint64_t ktime_ns(void);

/* Blocks the current execution for `ns` nanoseconds. */
void ksleep_ns(uint64_t ns);

#endif
