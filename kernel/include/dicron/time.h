#ifndef _DICRON_TIME_H
#define _DICRON_TIME_H

#include <stdint.h>

/* Returns the number of milliseconds since boot. */
uint64_t ktime_ms(void);

/* Blocks the current execution for `ms` milliseconds. */
void ksleep_ms(uint32_t ms);

#endif
