#include "ktest.h"
#include "drivers/new/ata/ata.h"
#include <dicron/blkdev.h>
#include "lib/string.h"

KTEST_REGISTER(test_partition_empty_table, "Part: empty partition table", KTEST_CAT_BOOT)
static void test_partition_empty_table(void)
{
	KTEST_BEGIN("Part: empty partition table");

	uint8_t data[1024];
	memset(data, 0, sizeof(data));

	/* Valid MBR signature but all partition entries zeroed */
	data[510] = 0x55;
	data[511] = 0xAA;

	struct blkdev *dev = ramdisk_create(data, sizeof(data), 512);
	if (!dev) return;

	struct partition_info parts[4];
	memset(parts, 0, sizeof(parts));

	int count = partition_scan(dev, parts, 4);
	KTEST_EQ(count, 0, "no partitions in empty table");
}
