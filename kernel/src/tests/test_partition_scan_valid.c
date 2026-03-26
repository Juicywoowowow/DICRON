#include "ktest.h"
#include "drivers/new/ata/ata.h"
#include <dicron/blkdev.h>
#include "lib/string.h"

KTEST_REGISTER(test_partition_scan_valid, "Part: scan valid MBR", KTEST_CAT_BOOT)
static void test_partition_scan_valid(void)
{
	KTEST_BEGIN("Part: scan valid MBR");

	uint8_t data[1024];
	memset(data, 0, sizeof(data));

	/* MBR signature */
	data[510] = 0x55;
	data[511] = 0xAA;

	/* Partition entry 0 at offset 446 */
	struct mbr_entry *e = (struct mbr_entry *)(data + 446);
	e->type = MBR_PART_TYPE_LINUX;
	e->lba_start = 2048;
	e->sector_count = 8192;

	struct blkdev *dev = ramdisk_create(data, sizeof(data), 512);
	if (!dev) return;

	struct partition_info parts[4];
	memset(parts, 0, sizeof(parts));

	int count = partition_scan(dev, parts, 4);
	KTEST_EQ(count, 1, "found 1 partition");
	KTEST_EQ(parts[0].valid, 1, "partition valid");
	KTEST_EQ(parts[0].type, MBR_PART_TYPE_LINUX, "type 0x83");
	KTEST_EQ(parts[0].start_lba, 2048, "start_lba");
	KTEST_EQ(parts[0].sector_count, 8192, "sector_count");
}
