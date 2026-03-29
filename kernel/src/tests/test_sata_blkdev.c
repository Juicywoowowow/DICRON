#include "ktest.h"
#include "../drivers/new/sata/sata.h"
#include <dicron/blkdev.h>

KTEST_REGISTER(ktest_sata_blkdev, "sata: blkdev fields valid", KTEST_CAT_BOOT)
static void ktest_sata_blkdev(void)
{
	KTEST_BEGIN("sata: blkdev fields valid");
	if (sata_drive_count() == 0) {
		KTEST_TRUE(1, "sata: no drives (skip)");
		return;
	}
	struct blkdev *bd = sata_blkdev_create(sata_get_drive(0));
	KTEST_NOT_NULL(bd, "sata: blkdev created");
	KTEST_NOT_NULL(bd->read, "sata: blkdev read op present");
	KTEST_NOT_NULL(bd->write, "sata: blkdev write op present");
	KTEST(bd->block_size == 512, "sata: block size is 512");
	KTEST(bd->total_blocks > (uint64_t)0, "sata: capacity > 0");
}
