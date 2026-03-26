#include "ktest.h"
#include "drivers/ata/ata.h"
#include "lib/string.h"

KTEST_REGISTER(test_ata_pio_boundary, "ATA: PIO boundary patterns", KTEST_CAT_BOOT)
static void test_ata_pio_boundary(void)
{
	KTEST_BEGIN("ATA: PIO boundary patterns");

	if (ata_drive_count() == 0)
		return;

	struct ata_drive *drv = ata_get_drive(0);
	if (!drv) return;

	uint64_t test_lba = drv->lba28_sectors - 4;
	uint8_t wbuf[512];
	uint8_t rbuf[512];

	/* All zeros */
	memset(wbuf, 0x00, 512);
	ata_pio_write(drv, test_lba, 1, wbuf);
	ata_pio_read(drv, test_lba, 1, rbuf);
	KTEST_TRUE(memcmp(wbuf, rbuf, 512) == 0, "all-zero pattern");

	/* All ones */
	memset(wbuf, 0xFF, 512);
	ata_pio_write(drv, test_lba, 1, wbuf);
	ata_pio_read(drv, test_lba, 1, rbuf);
	KTEST_TRUE(memcmp(wbuf, rbuf, 512) == 0, "all-0xFF pattern");

	/* Alternating */
	for (int i = 0; i < 512; i++)
		wbuf[i] = (uint8_t)((i % 2) ? 0xAA : 0x55);
	ata_pio_write(drv, test_lba, 1, wbuf);
	ata_pio_read(drv, test_lba, 1, rbuf);
	KTEST_TRUE(memcmp(wbuf, rbuf, 512) == 0, "alternating pattern");
}
