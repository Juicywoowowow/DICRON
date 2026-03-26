#include "ktest.h"
#include "drivers/new/ata/ata.h"
#include <dicron/blkdev.h>
#include "lib/string.h"

KTEST_REGISTER(test_partition_blkdev_rw, "Part: blkdev r/w offset", KTEST_CAT_BOOT)
static void test_partition_blkdev_rw(void)
{
	KTEST_BEGIN("Part: blkdev r/w offset");

	uint8_t data[4096];
	memset(data, 0, sizeof(data));

	/* Pre-fill sector 2 with known pattern */
	memset(data + 2 * 512, 0xAA, 512);

	struct blkdev *disk = ramdisk_create(data, sizeof(data), 512);
	if (!disk) return;

	/* Partition starts at sector 2, 4 sectors long */
	struct blkdev *part = partition_blkdev_create(disk, 2, 4);
	if (!part) return;

	/* Reading block 0 of partition = sector 2 of disk */
	uint8_t rbuf[512];
	int ret = part->read(part, 0, rbuf);
	KTEST_EQ(ret, 0, "read part block 0");

	uint8_t expected[512];
	memset(expected, 0xAA, 512);
	KTEST_TRUE(memcmp(rbuf, expected, 512) == 0,
		   "block 0 = disk sector 2");

	/* Write to partition block 1 = disk sector 3 */
	uint8_t wbuf[512];
	memset(wbuf, 0xBB, 512);
	ret = part->write(part, 1, wbuf);
	KTEST_EQ(ret, 0, "write part block 1");

	/* Verify via raw disk read of sector 3 */
	ret = disk->read(disk, 3, rbuf);
	KTEST_EQ(ret, 0, "raw read sector 3");
	KTEST_TRUE(memcmp(rbuf, wbuf, 512) == 0,
		   "partition write offset correct");
}
