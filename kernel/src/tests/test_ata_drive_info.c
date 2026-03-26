#include "ktest.h"
#include "drivers/ata/ata.h"
#include "lib/string.h"

KTEST_REGISTER(test_ata_drive_info, "ATA: drive info validation", KTEST_CAT_BOOT)
static void test_ata_drive_info(void)
{
	KTEST_BEGIN("ATA: drive info validation");

	int count = ata_drive_count();
	for (int i = 0; i < count; i++) {
		struct ata_drive *d = ata_get_drive(i);
		KTEST_NOT_NULL(d, "drive not NULL");
		if (!d) continue;

		KTEST_EQ(d->present, 1, "drive present");
		KTEST_TRUE(d->bus == 0 || d->bus == 1, "valid bus");
		KTEST_TRUE(d->drive == 0 || d->drive == 1, "valid drive");
		KTEST_TRUE(d->drive_sel == ATA_DRIVE_MASTER ||
			   d->drive_sel == ATA_DRIVE_SLAVE,
			   "valid drive_sel");
		KTEST_GT(d->lba28_sectors, 0, "nonzero sectors");
		KTEST_GT(strlen(d->model), 0, "nonempty model");
	}
}
