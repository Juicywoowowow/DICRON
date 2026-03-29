#include "ktest.h"
#include "../drivers/new/sata/sata.h"

KTEST_REGISTER(ktest_sata_drive_count, "sata: drive count non-negative", KTEST_CAT_BOOT)
static void ktest_sata_drive_count(void)
{
	KTEST_BEGIN("sata: drive count non-negative");
	int count = sata_drive_count();
	KTEST_GE(count, 0, "sata: count >= 0");
}

KTEST_REGISTER(ktest_sata_identify, "sata: IDENTIFY fields valid", KTEST_CAT_BOOT)
static void ktest_sata_identify(void)
{
	KTEST_BEGIN("sata: IDENTIFY fields valid");
	if (sata_drive_count() == 0) {
		KTEST_TRUE(1, "sata: no drives (skip)");
		return;
	}
	struct sata_device *dev = sata_get_drive(0);
	KTEST_NOT_NULL(dev, "sata: drive[0] non-null");
	KTEST_TRUE(dev->present, "sata: drive[0] present");
	KTEST(dev->total_sectors > (uint64_t)0, "sata: capacity > 0");
	KTEST(dev->model[0] != '\0', "sata: model string non-empty");
}
