#include "ktest.h"
#include "drivers/new/ata/ata.h"
#include "lib/string.h"

KTEST_REGISTER(test_partition_mbr_struct, "Part: MBR struct layout", KTEST_CAT_BOOT)
static void test_partition_mbr_struct(void)
{
	KTEST_BEGIN("Part: MBR struct layout");

	KTEST_EQ(sizeof(struct mbr_entry), 16, "mbr_entry is 16 bytes");

	struct mbr_entry e;
	memset(&e, 0, sizeof(e));
	e.type = 0x83;
	e.lba_start = 2048;
	e.sector_count = 32768;

	KTEST_EQ(e.type, 0x83, "type field");
	KTEST_EQ(e.lba_start, 2048, "lba_start field");
	KTEST_EQ(e.sector_count, 32768, "sector_count field");

	struct partition_info pi;
	memset(&pi, 0, sizeof(pi));
	KTEST_EQ(pi.valid, 0, "zeroed valid");
	KTEST_EQ(pi.type, 0, "zeroed type");
	KTEST_EQ(pi.start_lba, 0, "zeroed start_lba");
	KTEST_EQ(pi.sector_count, 0, "zeroed sector_count");
}
