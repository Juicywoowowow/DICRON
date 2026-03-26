#include "ktest.h"
#include "drivers/timer/rtc.h"

KTEST_REGISTER(test_rtc_unix, "RTC: unix timestamp", KTEST_CAT_BOOT)
static void test_rtc_unix(void)
{
	KTEST_BEGIN("RTC: unix timestamp");

	uint64_t ts = rtc_unix_time();

	/* 2024-01-01 00:00:00 UTC = 1704067200 */
	KTEST_GT(ts, 1704067200ULL, "after 2024-01-01");
	/* 2100-01-01 00:00:00 UTC = 4102444800 */
	KTEST_LT(ts, 4102444800ULL, "before 2100-01-01");

	/* Two reads should be close (within 2 seconds) */
	uint64_t ts2 = rtc_unix_time();
	KTEST_GE(ts2, ts, "monotonic");
	KTEST_LT(ts2 - ts, 2, "reads within 2s");
}
