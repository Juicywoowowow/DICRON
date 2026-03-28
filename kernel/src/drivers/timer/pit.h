#ifndef _DICRON_DRIVERS_TIMER_PIT_H
#define _DICRON_DRIVERS_TIMER_PIT_H

#include <stdint.h>
#include <generated/autoconf.h>

#ifdef CONFIG_PIT
/* Initializes the Programmable Interval Timer to 1000 Hz. */
void pit_init(void);

uint64_t pit_jiffies(void);
void pit_sleep_ms(uint32_t ms);
#else
static inline void pit_init(void) {}
static inline uint64_t pit_jiffies(void) { return 0; }
static inline void pit_sleep_ms(uint32_t ms) { (void)ms; }
#endif

#endif
