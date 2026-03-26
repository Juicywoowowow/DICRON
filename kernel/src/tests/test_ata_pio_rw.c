#include "ktest.h"
#include "drivers/new/ata/ata.h"
#include "lib/string.h"

KTEST_REGISTER(test_ata_pio_rw, "ATA: PIO read/write roundtrip", KTEST_CAT_BOOT)
static void test_ata_pio_rw(void)
{
	KTEST_BEGIN("ATA: PIO read/write roundtrip");

	if (ata_drive_count() == 0)
		return;

	struct ata_drive *drv = ata_get_drive(0);
	KTEST_NOT_NULL(drv, "have drive");
	if (!drv) return;

	/* Use a high LBA to avoid clobbering MBR/partition data */
	uint64_t test_lba = drv->lba28_sectors - 4;

	uint8_t wbuf[512];
	uint8_t rbuf[512];

	for (int i = 0; i < 512; i++)
		wbuf[i] = (uint8_t)(i & 0xFF);

	int ret = ata_pio_write(drv, test_lba, 1, wbuf);
	KTEST_EQ(ret, 0, "write sector ok");

	memset(rbuf, 0, sizeof(rbuf));
	ret = ata_pio_read(drv, test_lba, 1, rbuf);
	KTEST_EQ(ret, 0, "read sector ok");

	KTEST_TRUE(memcmp(wbuf, rbuf, 512) == 0, "data matches");
}
