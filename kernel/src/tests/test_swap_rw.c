/*
 * test_swap_rw.c — swap_page_out + swap_page_in roundtrip; data integrity
 */

#include "ktest.h"
#include "mm/swap.h"
#include "mm/pmm.h"
#include "lib/string.h"
#include <dicron/blkdev.h>

static uint8_t rw_disk_buf[SWAP_MAX_SLOTS * SWAP_SECTORS_PER_PAGE * 512];

KTEST_REGISTER(test_swap_rw, "Swap: page out/in roundtrip", KTEST_CAT_BOOT)
static void test_swap_rw(void)
{
	KTEST_BEGIN("Swap: page out/in roundtrip");

	memset(rw_disk_buf, 0, sizeof(rw_disk_buf));

	struct blkdev *disk = ramdisk_create(rw_disk_buf, sizeof(rw_disk_buf), 512);
	KTEST_NOT_NULL(disk, "ramdisk created for swap rw");
	if (!disk)
		return;

	swap_init(disk);
	KTEST_EQ(swap_is_enabled(), 1, "disk swap enabled");

	/* Write a page with known pattern */
	uint8_t original[4096];
	for (int i = 0; i < 4096; i++)
		original[i] = (uint8_t)(i & 0xFF);

	int slot = swap_page_out(original);
	KTEST_GE(slot, 0, "page_out succeeded");
	if (slot < 0)
		return;

	/* Read it back */
	uint8_t restored[4096];
	memset(restored, 0, sizeof(restored));

	int ret = swap_page_in(slot, restored);
	KTEST_EQ(ret, 0, "page_in succeeded");

	/* Verify data integrity */
	int match = (memcmp(original, restored, 4096) == 0);
	KTEST_TRUE(match, "roundtrip data integrity");

	/* Test with mostly-zero page */
	uint8_t zeros[4096];
	memset(zeros, 0, sizeof(zeros));
	zeros[0] = 0x42;
	zeros[4095] = 0x99;

	int slot2 = swap_page_out(zeros);
	KTEST_GE(slot2, 0, "zero-page out succeeded");
	if (slot2 < 0)
		return;

	uint8_t zbuf[4096];
	memset(zbuf, 0xFF, sizeof(zbuf));
	ret = swap_page_in(slot2, zbuf);
	KTEST_EQ(ret, 0, "zero-page in succeeded");
	KTEST_TRUE(memcmp(zeros, zbuf, 4096) == 0, "zero-page integrity");

	swap_free_slot(slot);
	swap_free_slot(slot2);
}
