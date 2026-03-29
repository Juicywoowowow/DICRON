#ifndef _DICRON_DRIVERS_HPET_UTIL_H
#define _DICRON_DRIVERS_HPET_UTIL_H

#include <stdint.h>

#ifdef CONFIG_HPET
/* Utility functions for interacting with HPET hardware */
uint64_t hpet_read_main_counter(void);
uint64_t hpet_get_period_fs(void);
void hpet_set_main_counter(uint64_t val);
void hpet_enable(void);
#endif

#endif
