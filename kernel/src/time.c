#include <dicron/time.h>
#include <generated/autoconf.h>

#ifdef CONFIG_HPET
#include "drivers/hpet/hpet.h"
#endif

#ifdef CONFIG_PIT
#include "drivers/timer/pit.h"
#endif

uint64_t ktime_ns(void)
{
#ifdef CONFIG_HPET
	if (hpet_is_available())
		return hpet_ktime_ns();
#endif
	return ktime_ms() * 1000000ULL;
}

void ksleep_ns(uint64_t ns)
{
#ifdef CONFIG_HPET
	if (hpet_is_available()) {
		hpet_ksleep_ns(ns);
		return;
	}
#endif
	ksleep_ms((uint32_t)(ns / 1000000ULL));
}

uint64_t ktime_ms(void)
{
#ifdef CONFIG_HPET
	if (hpet_is_available())
		return hpet_ktime_ns() / 1000000ULL;
#endif
#ifdef CONFIG_PIT
	return pit_jiffies();
#else
	return 0;
#endif
}

void ksleep_ms(uint32_t ms)
{
#ifdef CONFIG_HPET
	if (hpet_is_available()) {
		hpet_ksleep_ns((uint64_t)ms * 1000000ULL);
		return;
	}
#endif
#ifdef CONFIG_PIT
	pit_sleep_ms(ms);
#endif
}
