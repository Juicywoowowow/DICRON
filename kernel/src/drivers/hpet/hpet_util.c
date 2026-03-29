#include "hpet_util.h"
#include <generated/autoconf.h>
#include "hpet_internal.h"
#include <dicron/log.h>

#ifdef CONFIG_HPET

uint64_t hpet_read_main_counter(void)
{
	if (!hpet_base)
		return 0;
	return *(volatile uint64_t *)(hpet_base + HPET_REG_MAIN_COUNTER);
}

void hpet_set_main_counter(uint64_t val)
{
	if (!hpet_base)
		return;
	*(volatile uint64_t *)(hpet_base + HPET_REG_MAIN_COUNTER) = val;
}

uint64_t hpet_get_period_fs(void)
{
	if (!hpet_base)
		return 0;
	uint64_t cap = *(volatile uint64_t *)(hpet_base + HPET_REG_CAPABILITIES);
	return (cap >> 32) & 0xFFFFFFFF;
}

void hpet_enable(void)
{
	if (!hpet_base)
		return;
	uint64_t cfg = *(volatile uint64_t *)(hpet_base + HPET_REG_CONFIG);
	cfg |= 1; /* Enable */
	*(volatile uint64_t *)(hpet_base + HPET_REG_CONFIG) = cfg;
}

#endif
