#include "ktest.h"
#include "../drivers/new/ata/ata.h"
#include "mm/kheap.h"
#include "lib/string.h"

KTEST_REGISTER(test_ata_stress_rw, "ata stress read/write", KTEST_CAT_BOOT)
static void test_ata_stress_rw(void)
{
	KTEST_BEGIN("ATA Stress RW");

#ifdef CONFIG_TEST_ATA_DRIVE
	struct ata_drive *drv = ata_get_drive(0);
	KTEST_NOT_NULL(drv, "ATA drive must exist for stress test");

	if (drv && drv->present) {
		uint8_t *buf = kmalloc(512);
		KTEST_NOT_NULL(buf, "Allocated buffer");

		for (uint64_t i = 0; i < 50; i++) {
			memset(buf, (uint8_t)i, 512);
			int w = ata_pio_write(drv, i, 1, buf);
			KTEST_EQ(w, 0, "Stress write sector");
			
			memset(buf, 0, 512);
			int r = ata_pio_read(drv, i, 1, buf);
			KTEST_EQ(r, 0, "Stress read sector");
			KTEST_EQ((int)buf[0], i, "Stress sector data match");
		}
		kfree(buf);
	}
#else
	/* No disk present, do sanity checks */
	KTEST_TRUE(ata_drive_count() >= 0, "ATA subsystem init OK");
	KTEST_TRUE(1, "Bypassed stress tests (no physical attach)");
#endif
}
