#include "ktest.h"
#include "../drivers/new/hpet/hpet_util.h"

#ifdef CONFIG_HPET
KTEST_REGISTER(ktest_hpet_counter, "hpet counter", KTEST_CAT_BOOT)
static void ktest_hpet_counter(void)
{
	KTEST_BEGIN("HPET counter monotonicity");
	uint64_t v1 = hpet_read_main_counter();
	for (volatile int i = 0; i < 10000; i++) {}
	uint64_t v2 = hpet_read_main_counter();
	KTEST_GE(v2, v1, "HPET counter should never decrease");
}
#endif
