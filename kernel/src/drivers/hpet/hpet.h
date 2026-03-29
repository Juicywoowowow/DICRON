#ifndef _DICRON_DRIVERS_HPET_H
#define _DICRON_DRIVERS_HPET_H

#include <stdint.h>
#include <generated/autoconf.h>

#ifdef CONFIG_HPET
/**
 * hpet_init() - Initialize the High Precision Event Timer.
 * 
 * Maps the HPET MMIO region, determines the timer period, configures
 * the main counter, and sets up Timer 0 for legacy replacement routing
 * to generate periodic 1ms interrupts that drive the kernel scheduler.
 * On failure, ktime falls back to 0 or panics if critical.
 */
void hpet_init(void);

/**
 * hpet_is_available() - Check if the HPET was successfully initialized.
 * 
 * Return: non-zero if the HPET timer is active and readable, 0 otherwise.
 */
int hpet_is_available(void);

uint64_t hpet_ktime_ns(void);
void hpet_ksleep_ns(uint64_t ns);
#else
static inline void hpet_init(void) {}
static inline int hpet_is_available(void) { return 0; }
#endif

#endif
