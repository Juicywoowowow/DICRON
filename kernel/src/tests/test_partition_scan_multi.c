#include "ktest.h"
#include "drivers/ata/ata.h"
#include <dicron/blkdev.h>
#include "lib/string.h"

KTEST_REGISTER(test_partition_scan_multi, "Part: scan multiple partitions", KTEST_CAT_BOOT)
static void test_partition_scan_multi(void)
{
	KTEST_BEGIN("Part: scan multiple partitions");

	uint8_t data[1024];
	memset(data, 0, sizeof(data));
	data[510] = 0x55;
	data[511] = 0xAA;

	/* 3 partitions */
	struct mbr_entry *e0 = (struct mbr_entry *)(data + 446);
	e0->type = MBR_PART_TYPE_LINUX;
	e0->lba_start = 2048;
	e0->sector_count = 4096;

	struct mbr_entry *e1 = (struct mbr_entry *)(data + 446 + 16);
	e1->type = 0x82; /* swap */
	e1->lba_start = 6144;
	e1->sector_count = 2048;

	struct mbr_entry *e2 = (struct mbr_entry *)(data + 446 + 32);
	e2->type = MBR_PART_TYPE_LINUX;
	e2->lba_start = 8192;
	e2->sector_count = 16384;

	/* Entry 3 left empty (type 0) */

	struct blkdev *dev = ramdisk_create(data, sizeof(data), 512);
	if (!dev) return;

	struct partition_info parts[4];
	memset(parts, 0, sizeof(parts));

	int count = partition_scan(dev, parts, 4);
	KTEST_EQ(count, 3, "found 3 partitions");
	KTEST_EQ(parts[0].type, MBR_PART_TYPE_LINUX, "p0 linux");
	KTEST_EQ(parts[1].type, 0x82, "p1 swap");
	KTEST_EQ(parts[2].type, MBR_PART_TYPE_LINUX, "p2 linux");
	KTEST_EQ(parts[2].start_lba, 8192, "p2 start");
}
