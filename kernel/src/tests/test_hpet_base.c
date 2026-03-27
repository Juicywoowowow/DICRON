#include "ktest.h"
#include "../drivers/new/hpet/hpet_internal.h"

#ifdef CONFIG_HPET
KTEST_REGISTER(ktest_hpet_base, "hpet base pointer", KTEST_CAT_BOOT)
static void ktest_hpet_base(void)
{
	KTEST_BEGIN("HPET base mapping");
	KTEST_NOT_NULL((void *)hpet_base, "HPET base should be mapped");
}
#endif
