#include "ktest.h"
#include <dicron/blkdev.h>
#include "lib/string.h"

static void test_blkdev_ramdisk(void)
{
	/* Create a 4 KiB ramdisk with 1024-byte blocks */
	char data[4096];
	memset(data, 0, sizeof(data));
	memcpy(data, "BLOCK0", 6);
	memcpy(data + 1024, "BLOCK1", 6);
	memcpy(data + 2048, "BLOCK2", 6);

	struct blkdev *dev = ramdisk_create(data, sizeof(data), 1024);
	KTEST_NOT_NULL(dev, "ramdisk_create succeeds");
	if (!dev) return;

	KTEST_EQ((long long)dev->block_size, 1024, "block_size is 1024");
	KTEST_EQ((long long)dev->total_blocks, 4, "total_blocks is 4");

	/* Read block 0 */
	char buf[1024];
	int ret = dev->read(dev, 0, buf);
	KTEST_EQ(ret, 0, "read block 0 ok");
	KTEST_TRUE(memcmp(buf, "BLOCK0", 6) == 0, "block 0 content");

	/* Read block 1 */
	ret = dev->read(dev, 1, buf);
	KTEST_EQ(ret, 0, "read block 1 ok");
	KTEST_TRUE(memcmp(buf, "BLOCK1", 6) == 0, "block 1 content");

	/* Write to block 3 and read back */
	char wdata[1024];
	memset(wdata, 0, sizeof(wdata));
	memcpy(wdata, "WRITTEN", 7);
	ret = dev->write(dev, 3, wdata);
	KTEST_EQ(ret, 0, "write block 3 ok");

	ret = dev->read(dev, 3, buf);
	KTEST_EQ(ret, 0, "read block 3 ok");
	KTEST_TRUE(memcmp(buf, "WRITTEN", 7) == 0, "block 3 written content");

	/* Out-of-bounds read should fail */
	ret = dev->read(dev, 5, buf);
	KTEST_EQ(ret, -1, "OOB read fails");
}

KTEST_REGISTER(test_blkdev_ramdisk, "Blkdev: ramdisk read/write", KTEST_CAT_BOOT)
