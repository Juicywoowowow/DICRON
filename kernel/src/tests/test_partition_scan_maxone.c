#include "ktest.h"
#include "drivers/ata/ata.h"
#include <dicron/blkdev.h>
#include "lib/string.h"

KTEST_REGISTER(test_partition_scan_maxone, "Part: scan max=1 limit", KTEST_CAT_BOOT)
static void test_partition_scan_maxone(void)
{
	KTEST_BEGIN("Part: scan max=1 limit");

	uint8_t data[1024];
	memset(data, 0, sizeof(data));
	data[510] = 0x55;
	data[511] = 0xAA;

	/* Two valid partitions */
	struct mbr_entry *e0 = (struct mbr_entry *)(data + 446);
	e0->type = MBR_PART_TYPE_LINUX;
	e0->lba_start = 2048;
	e0->sector_count = 4096;

	struct mbr_entry *e1 = (struct mbr_entry *)(data + 446 + 16);
	e1->type = MBR_PART_TYPE_LINUX;
	e1->lba_start = 8192;
	e1->sector_count = 8192;

	struct blkdev *dev = ramdisk_create(data, sizeof(data), 512);
	if (!dev) return;

	struct partition_info parts[4];
	memset(parts, 0, sizeof(parts));

	/* Only request max 1 */
	int count = partition_scan(dev, parts, 1);
	KTEST_EQ(count, 1, "capped at max=1");
	KTEST_EQ(parts[0].start_lba, 2048, "got first partition");
	KTEST_EQ(parts[1].valid, 0, "second slot untouched");
}
