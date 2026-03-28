#include "ktest.h"
#include "../drivers/new/ata/ata.h"
#include "mm/kheap.h"

KTEST_REGISTER(test_ata_invalid_lba, "ata invalid lba", KTEST_CAT_BOOT)
static void test_ata_invalid_lba(void)
{
	KTEST_BEGIN("ATA Invalid LBA handles correctly");

#ifdef CONFIG_TEST_ATA_DRIVE
	struct ata_drive *drv = ata_get_drive(0);
	KTEST_NOT_NULL(drv, "Drive exists");

	if (drv && drv->present) {
		uint8_t *buf = kzalloc(512);
		uint64_t bad_lba = 0xFFFFFFFFFFFFFULL; /* extremely huge LBA */
		
		int r = ata_pio_read(drv, bad_lba, 1, buf);
		KTEST_LT(r, 0, "Read out of bounds LBA fails gracefully");
		
		int w = ata_pio_write(drv, bad_lba, 1, buf);
		KTEST_LT(w, 0, "Write out of bounds LBA fails gracefully");
		
		kfree(buf);
	}
#else
	KTEST_TRUE(1, "Bypassed invalid LBA tests (no disk)");
#endif
}
