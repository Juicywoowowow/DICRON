#include "ktest.h"
#include <dicron/time.h>

KTEST_REGISTER(ktest_timer_edge, "timer edge", KTEST_CAT_BOOT)
static void ktest_timer_edge(void)
{
	KTEST_BEGIN("timer edge cases");

	/* ksleep_ms(0) should not block */
	uint64_t t1 = ktime_ms();
	ksleep_ms(0);
	uint64_t t2 = ktime_ms();
	KTEST_LT(t2 - t1, 50, "ksleep_ms(0) returns immediately");

	/* timer advances during busy loop */
	uint64_t t3 = ktime_ms();
	while (ktime_ms() - t3 < 25)
		;
	KTEST_TRUE(1, "timer advances during busy loop");

	/* ktime_ms monotonically increasing */
	uint64_t prev = ktime_ms();
	for (int i = 0; i < 100; i++) {
		uint64_t now = ktime_ms();
		KTEST_GE(now, prev, "ktime_ms monotonic");
		prev = now;
	}
}
