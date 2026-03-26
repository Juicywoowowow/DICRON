#include "ktest.h"
#include "drivers/new/ata/ata.h"
#include <dicron/blkdev.h>

KTEST_REGISTER(test_ata_blkdev_create, "ATA: blkdev create", KTEST_CAT_BOOT)
static void test_ata_blkdev_create(void)
{
	KTEST_BEGIN("ATA: blkdev create");

	KTEST_NULL(ata_blkdev_create(NULL), "NULL drv returns NULL");

	struct ata_drive fake;
	__builtin_memset(&fake, 0, sizeof(fake));
	KTEST_NULL(ata_blkdev_create(&fake), "non-present returns NULL");

	if (ata_drive_count() == 0)
		return;

	struct ata_drive *drv = ata_get_drive(0);
	struct blkdev *bdev = ata_blkdev_create(drv);
	KTEST_NOT_NULL(bdev, "blkdev created");
	if (!bdev) return;

	KTEST_EQ(bdev->block_size, ATA_SECTOR_SIZE, "block_size 512");
	KTEST_EQ(bdev->total_blocks, drv->lba28_sectors,
		 "total_blocks matches");
	KTEST_NOT_NULL((void *)(uintptr_t)bdev->read, "read fn set");
	KTEST_NOT_NULL((void *)(uintptr_t)bdev->write, "write fn set");
}
