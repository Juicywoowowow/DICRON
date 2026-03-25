#ifndef _DICRON_DRIVERS_TIMER_PIT_H
#define _DICRON_DRIVERS_TIMER_PIT_H

#include <stdint.h>
#include <generated/autoconf.h>

#ifdef CONFIG_PIT
/* Initializes the Programmable Interval Timer to 1000 Hz. */
void pit_init(void);
#else
static inline void pit_init(void) {}
#endif

#endif
