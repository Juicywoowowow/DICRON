#include "ktest.h"
#include "drivers/new/ata/ata.h"
#include <dicron/blkdev.h>

KTEST_REGISTER(test_partition_blkdev_create, "Part: blkdev create", KTEST_CAT_BOOT)
static void test_partition_blkdev_create(void)
{
	KTEST_BEGIN("Part: blkdev create");

	KTEST_NULL(partition_blkdev_create(NULL, 0, 0), "NULL disk");

	uint8_t data[4096];
	__builtin_memset(data, 0, sizeof(data));
	struct blkdev *disk = ramdisk_create(data, sizeof(data), 512);
	if (!disk) return;

	struct blkdev *part = partition_blkdev_create(disk, 2, 4);
	KTEST_NOT_NULL(part, "partition blkdev created");
	if (!part) return;

	KTEST_EQ(part->block_size, 512, "inherits block_size");
	KTEST_EQ(part->total_blocks, 4, "total_blocks = sectors");
	KTEST_NOT_NULL((void *)(uintptr_t)part->read, "read fn");
	KTEST_NOT_NULL((void *)(uintptr_t)part->write, "write fn");
}
