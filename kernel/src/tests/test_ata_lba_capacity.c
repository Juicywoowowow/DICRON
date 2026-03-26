#include "ktest.h"
#include "drivers/ata/ata.h"

KTEST_REGISTER(test_ata_lba_capacity, "ATA: LBA capacity check", KTEST_CAT_BOOT)
static void test_ata_lba_capacity(void)
{
	KTEST_BEGIN("ATA: LBA capacity check");

	if (ata_drive_count() == 0)
		return;

	struct ata_drive *drv = ata_get_drive(0);
	if (!drv) return;

	/* LBA28 max is 2^28 = 268435456 sectors */
	KTEST_GT(drv->lba28_sectors, 0, "has sectors");
	KTEST_LT(drv->lba28_sectors, 268435457,
		  "lba28 within 28-bit range");

	uint64_t size_mb = (uint64_t)drv->lba28_sectors * 512 / (1024 * 1024);
	KTEST_GT(size_mb, 0, "nonzero MB capacity");

	if (drv->lba48) {
		KTEST_GT(drv->lba48_sectors, 0, "lba48 has sectors");
		KTEST_GE(drv->lba48_sectors, drv->lba28_sectors,
			 "lba48 >= lba28");
	}
}
