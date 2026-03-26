#include "ktest.h"
#include "drivers/ata/ata.h"
#include <dicron/blkdev.h>
#include "lib/string.h"

KTEST_REGISTER(test_ata_blkdev_rw, "ATA: blkdev read/write", KTEST_CAT_BOOT)
static void test_ata_blkdev_rw(void)
{
	KTEST_BEGIN("ATA: blkdev read/write");

	if (ata_drive_count() == 0)
		return;

	struct blkdev *bdev = ata_blkdev_create(ata_get_drive(0));
	if (!bdev) return;

	uint64_t test_block = bdev->total_blocks - 2;
	uint8_t wbuf[512];
	uint8_t rbuf[512];

	for (int i = 0; i < 512; i++)
		wbuf[i] = (uint8_t)(i ^ 0x42);

	int ret = bdev->write(bdev, test_block, wbuf);
	KTEST_EQ(ret, 0, "blkdev write ok");

	memset(rbuf, 0, 512);
	ret = bdev->read(bdev, test_block, rbuf);
	KTEST_EQ(ret, 0, "blkdev read ok");

	KTEST_TRUE(memcmp(wbuf, rbuf, 512) == 0, "blkdev data matches");
}
