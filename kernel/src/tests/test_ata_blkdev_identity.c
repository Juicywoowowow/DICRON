#include "ktest.h"
#include "drivers/ata/ata.h"
#include <dicron/blkdev.h>
#include "lib/string.h"

KTEST_REGISTER(test_ata_blkdev_identity, "ATA: blkdev write-read identity", KTEST_CAT_BOOT)
static void test_ata_blkdev_identity(void)
{
	KTEST_BEGIN("ATA: blkdev write-read identity");

	if (ata_drive_count() == 0)
		return;

	struct blkdev *bdev = ata_blkdev_create(ata_get_drive(0));
	if (!bdev) return;

	uint64_t blk = bdev->total_blocks - 2;
	uint8_t w1[512], w2[512], rbuf[512];

	memset(w1, 0xAB, 512);
	memset(w2, 0xCD, 512);

	bdev->write(bdev, blk, w1);
	bdev->read(bdev, blk, rbuf);
	KTEST_TRUE(memcmp(w1, rbuf, 512) == 0, "first write");

	bdev->write(bdev, blk, w2);
	bdev->read(bdev, blk, rbuf);
	KTEST_TRUE(memcmp(w2, rbuf, 512) == 0, "overwrite");

	/* Verify w1 is gone */
	KTEST_FALSE(memcmp(w1, rbuf, 512) == 0, "old data replaced");
}
