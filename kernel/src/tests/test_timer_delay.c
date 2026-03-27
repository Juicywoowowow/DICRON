#include "ktest.h"
#include <dicron/time.h>

KTEST_REGISTER(ktest_timer_delay, "timer delay", KTEST_CAT_BOOT)
static void ktest_timer_delay(void)
{
	KTEST_BEGIN("timer delays");

	uint64_t t1 = ktime_ms();
	ksleep_ms(250);
	uint64_t t2 = ktime_ms();
	KTEST_GE(t2 - t1, 250, "250ms sleep >= 250ms");
	KTEST_LT(t2 - t1, 500, "250ms sleep < 500ms (not stuck)");

	uint64_t t3 = ktime_ms();
	ksleep_ms(1000);
	uint64_t t4 = ktime_ms();
	KTEST_GE(t4 - t3, 1000, "1000ms sleep >= 1000ms");
	KTEST_LT(t4 - t3, 1500, "1000ms sleep < 1500ms");
}
