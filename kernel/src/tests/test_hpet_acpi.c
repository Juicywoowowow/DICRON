#include "ktest.h"
#include <dicron/acpi.h>

#ifdef CONFIG_HPET
KTEST_REGISTER(ktest_hpet_acpi, "hpet acpi table", KTEST_CAT_BOOT)
static void ktest_hpet_acpi(void)
{
	KTEST_BEGIN("HPET ACPI table resolution");
	void *table = acpi_find_table("HPET");
	KTEST_NOT_NULL(table, "HPET ACPI table should be present");
}
#endif
