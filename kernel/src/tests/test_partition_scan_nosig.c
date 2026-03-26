#include "ktest.h"
#include "drivers/new/ata/ata.h"
#include <dicron/blkdev.h>
#include "lib/string.h"

KTEST_REGISTER(test_partition_scan_nosig, "Part: scan bad signature", KTEST_CAT_BOOT)
static void test_partition_scan_nosig(void)
{
	KTEST_BEGIN("Part: scan bad signature");

	uint8_t data[512];
	memset(data, 0, sizeof(data));

	struct blkdev *dev = ramdisk_create(data, 512, 512);
	if (!dev) return;

	struct partition_info parts[4];
	memset(parts, 0, sizeof(parts));

	int count = partition_scan(dev, parts, 4);
	KTEST_EQ(count, 0, "no signature -> 0 partitions");

	/* Bad signature */
	data[510] = 0x55;
	data[511] = 0xBB;
	count = partition_scan(dev, parts, 4);
	KTEST_EQ(count, 0, "bad sig -> 0 partitions");
}
