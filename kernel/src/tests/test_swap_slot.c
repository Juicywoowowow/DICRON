/*
 * test_swap_slot.c — Disk swap slot alloc/free, exhaustion, double-free
 */

#include "ktest.h"
#include "mm/swap.h"
#include "mm/pmm.h"
#include "lib/string.h"
#include <dicron/blkdev.h>

/*
 * Use a stack-allocated ramdisk backing buffer.
 * Need SWAP_MAX_SLOTS * 8 sectors * 512 bytes = 64 * 8 * 512 = 256 KiB
 * That's too large for stack — use a static buffer.
 */
static uint8_t slot_disk_buf[SWAP_MAX_SLOTS * SWAP_SECTORS_PER_PAGE * 512];

KTEST_REGISTER(test_swap_slot, "Swap: slot alloc/free/exhaust", KTEST_CAT_BOOT)
static void test_swap_slot(void)
{
	KTEST_BEGIN("Swap: slot alloc/free/exhaust");

	memset(slot_disk_buf, 0, sizeof(slot_disk_buf));

	struct blkdev *disk = ramdisk_create(slot_disk_buf, sizeof(slot_disk_buf), 512);
	KTEST_NOT_NULL(disk, "ramdisk created for swap slots");
	if (!disk)
		return;

	swap_init(disk);
	KTEST_EQ(swap_is_enabled(), 1, "disk swap enabled with ramdisk");

	/* Allocate all slots by writing pages */
	uint8_t page[4096];
	int slots[SWAP_MAX_SLOTS];
	int all_ok = 1;

	for (int i = 0; i < SWAP_MAX_SLOTS; i++) {
		memset(page, (uint8_t)i, sizeof(page));
		slots[i] = swap_page_out(page);
		if (slots[i] < 0) {
			all_ok = 0;
			break;
		}
	}
	KTEST_TRUE(all_ok, "all 64 slots allocated");

	/* Exhaustion: next alloc should fail */
	memset(page, 0xFF, sizeof(page));
	int overflow = swap_page_out(page);
	KTEST_EQ(overflow, -1, "slot exhaustion returns -1");

	/* Free a slot and re-allocate */
	swap_free_slot(slots[0]);
	memset(page, 0xAA, sizeof(page));
	int reuse = swap_page_out(page);
	KTEST_GE(reuse, 0, "freed slot can be reused");

	/* Double-free is a no-op (should not crash) */
	swap_free_slot(slots[1]);
	swap_free_slot(slots[1]);
	KTEST_TRUE(1, "double-free did not crash");

	/* Cleanup */
	for (int i = 2; i < SWAP_MAX_SLOTS; i++)
		swap_free_slot(slots[i]);
	if (reuse >= 0)
		swap_free_slot(reuse);
}
