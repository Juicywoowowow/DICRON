#include "ktest.h"
#include "drivers/new/ata/ata.h"

KTEST_REGISTER(test_ata_detect_api, "ATA: detect API bounds", KTEST_CAT_BOOT)
static void test_ata_detect_api(void)
{
	KTEST_BEGIN("ATA: detect API bounds");

	KTEST_GE(ata_drive_count(), 0, "drive count >= 0");
	KTEST_LT(ata_drive_count(), ATA_MAX_DRIVES + 1,
		  "drive count <= MAX_DRIVES");

	KTEST_NULL(ata_get_drive(-1), "negative index NULL");
	KTEST_NULL(ata_get_drive(ATA_MAX_DRIVES), "max index NULL");
	KTEST_NULL(ata_get_drive(100), "huge index NULL");

	if (ata_drive_count() > 0) {
		struct ata_drive *d = ata_get_drive(0);
		KTEST_NOT_NULL(d, "drive 0 exists");
		KTEST_EQ(d->present, 1, "drive 0 present");
		KTEST_GT(d->lba28_sectors, 0, "drive 0 has sectors");
	}
}
