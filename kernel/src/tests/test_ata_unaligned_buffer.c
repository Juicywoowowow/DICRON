#include "ktest.h"
#include "../drivers/new/ata/ata.h"
#include "mm/kheap.h"
#include "lib/string.h"

KTEST_REGISTER(test_ata_unaligned_buffer, "ata unaligned buffer", KTEST_CAT_BOOT)
static void test_ata_unaligned_buffer(void)
{
	KTEST_BEGIN("ATA Unaligned Buffer IO");

#ifdef CONFIG_TEST_ATA_DRIVE
	struct ata_drive *drv = ata_get_drive(0);
	KTEST_NOT_NULL(drv, "Drive exists");

	if (drv && drv->present) {
		/* Allocate a buffer and offset it by 1 byte to make it unaligned */
		uint8_t *alloc_buf = kzalloc(1024);
		uint8_t *buf = alloc_buf + 1;
		
		int r = ata_pio_read(drv, 0, 1, buf);
		KTEST_EQ(r, 0, "Ins/Outs handles unaligned buffers on x86");
		
		kfree(alloc_buf);
	}
#else
	KTEST_TRUE(1, "Bypassed unaligned buffer tests (no disk)");
#endif
}
