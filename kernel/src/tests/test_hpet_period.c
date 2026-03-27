#include "ktest.h"
#include "../drivers/new/hpet/hpet_util.h"

#ifdef CONFIG_HPET
KTEST_REGISTER(ktest_hpet_period, "hpet period", KTEST_CAT_BOOT)
static void ktest_hpet_period(void)
{
	KTEST_BEGIN("HPET period validity");
	uint64_t period = hpet_get_period_fs();
	KTEST_GT(period, 0ULL, "HPET period should be > 0");
	KTEST_LT(period, 0x05F5E100ULL, "HPET period should be bounded bounds");
}
#endif
