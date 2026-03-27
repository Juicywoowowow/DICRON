#include "ktest.h"
#include <dicron/time.h>

#ifdef CONFIG_HPET
KTEST_REGISTER(ktest_hpet_delay_ns, "hpet delay ns", KTEST_CAT_BOOT)
static void ktest_hpet_delay_ns(void)
{
	KTEST_BEGIN("HPET nanosecond delay");
	uint64_t t1 = ktime_ns();
	ksleep_ns(100000); /* 100 us */
	uint64_t t2 = ktime_ns();
	KTEST_GE(t2 - t1, 100000ULL, "HPET delay should take at least requested ns");
}
#endif
