#include "ktest.h"
#include "drivers/ata/ata.h"
#include <dicron/mem.h>
#include "lib/string.h"

KTEST_REGISTER(test_ata_pio_multi, "ATA: PIO multi-sector r/w", KTEST_CAT_BOOT)
static void test_ata_pio_multi(void)
{
	KTEST_BEGIN("ATA: PIO multi-sector r/w");

	if (ata_drive_count() == 0)
		return;

	struct ata_drive *drv = ata_get_drive(0);
	if (!drv) return;

	uint64_t test_lba = drv->lba28_sectors - 8;
	uint8_t *wbuf = kmalloc(512 * 4);
	uint8_t *rbuf = kmalloc(512 * 4);
	if (!wbuf || !rbuf) {
		if (wbuf) kfree(wbuf);
		if (rbuf) kfree(rbuf);
		return;
	}

	for (int i = 0; i < 512 * 4; i++)
		wbuf[i] = (uint8_t)((i * 7 + 13) & 0xFF);

	int ret = ata_pio_write(drv, test_lba, 4, wbuf);
	KTEST_EQ(ret, 0, "write 4 sectors");

	memset(rbuf, 0, 512 * 4);
	ret = ata_pio_read(drv, test_lba, 4, rbuf);
	KTEST_EQ(ret, 0, "read 4 sectors");

	KTEST_TRUE(memcmp(wbuf, rbuf, 512 * 4) == 0, "4-sector data ok");

	kfree(wbuf);
	kfree(rbuf);
}
