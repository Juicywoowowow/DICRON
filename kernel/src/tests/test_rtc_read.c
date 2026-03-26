#include "ktest.h"
#include "drivers/timer/rtc.h"

KTEST_REGISTER(test_rtc_read, "RTC: read time", KTEST_CAT_BOOT)
static void test_rtc_read(void)
{
	KTEST_BEGIN("RTC: read time");

	struct rtc_time t;
	rtc_read(&t);

	KTEST_LT(t.second, 60, "seconds < 60");
	KTEST_LT(t.minute, 60, "minutes < 60");
	KTEST_LT(t.hour, 24, "hours < 24");
	KTEST_GT(t.day, 0, "day > 0");
	KTEST_LT(t.day, 32, "day < 32");
	KTEST_GT(t.month, 0, "month > 0");
	KTEST_LT(t.month, 13, "month < 13");
	KTEST_GE(t.year, 2024, "year >= 2024");
	KTEST_LT(t.year, 2100, "year < 2100");
}
