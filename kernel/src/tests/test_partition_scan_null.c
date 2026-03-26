#include "ktest.h"
#include "drivers/new/ata/ata.h"
#include <dicron/blkdev.h>

KTEST_REGISTER(test_partition_scan_null, "Part: scan null args", KTEST_CAT_BOOT)
static void test_partition_scan_null(void)
{
	KTEST_BEGIN("Part: scan null args");

	struct partition_info parts[4];

	KTEST_EQ(partition_scan(NULL, parts, 4), 0, "NULL disk");
	KTEST_EQ(partition_scan(NULL, NULL, 4), 0, "NULL both");

	/* Create a tiny ramdisk with no valid MBR */
	uint8_t data[512];
	__builtin_memset(data, 0, sizeof(data));
	struct blkdev *dev = ramdisk_create(data, 512, 512);
	if (!dev) return;

	KTEST_EQ(partition_scan(dev, NULL, 4), 0, "NULL out");
	KTEST_EQ(partition_scan(dev, parts, 0), 0, "zero max");
}
