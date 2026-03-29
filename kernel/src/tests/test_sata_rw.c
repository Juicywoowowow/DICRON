#include "ktest.h"
#include "../drivers/new/sata/sata.h"
#include "mm/pmm.h"
#include "lib/string.h"

KTEST_REGISTER(ktest_sata_read, "sata: single-sector read", KTEST_CAT_BOOT)
static void ktest_sata_read(void)
{
	KTEST_BEGIN("sata: single-sector read");
	if (sata_drive_count() == 0) {
		KTEST_TRUE(1, "sata: no drives (skip)");
		return;
	}
	void *buf = pmm_alloc_page();
	KTEST_NOT_NULL(buf, "sata: alloc read buffer");
	memset(buf, 0xAA, 512);

	int ret = sata_read(sata_get_drive(0), 0, 1, buf);
	KTEST_EQ(ret, 0, "sata: read sector 0 succeeds");

	pmm_free_page(buf);
}

KTEST_REGISTER(ktest_sata_write_readback, "sata: write + readback", KTEST_CAT_BOOT)
static void ktest_sata_write_readback(void)
{
	KTEST_BEGIN("sata: write + readback");
	if (sata_drive_count() == 0) {
		KTEST_TRUE(1, "sata: no drives (skip)");
		return;
	}

	void *wbuf = pmm_alloc_page();
	void *rbuf = pmm_alloc_page();
	KTEST_NOT_NULL(wbuf, "sata: alloc write buffer");
	KTEST_NOT_NULL(rbuf, "sata: alloc read buffer");

	/* Write a known pattern to a high sector to avoid clobbering data */
	memset(wbuf, 0, PAGE_SIZE);
	uint8_t *w = (uint8_t *)wbuf;
	for (int i = 0; i < 512; i++)
		w[i] = (uint8_t)(i & 0xFF);

	struct sata_device *dev = sata_get_drive(0);
	uint64_t test_lba = dev->total_sectors - 1;

	int ret = sata_write(dev, test_lba, 1, wbuf);
	KTEST_EQ(ret, 0, "sata: write succeeds");

	memset(rbuf, 0, PAGE_SIZE);
	ret = sata_read(dev, test_lba, 1, rbuf);
	KTEST_EQ(ret, 0, "sata: readback succeeds");

	KTEST_EQ(memcmp(wbuf, rbuf, 512), 0, "sata: data matches");

	pmm_free_page(wbuf);
	pmm_free_page(rbuf);
}
