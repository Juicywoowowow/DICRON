/*
 * test_zram_oom.c — Fill ZRAM slots; verify graceful full-pool return; free and re-use
 */

#include "ktest.h"
#include "mm/zram.h"
#include "mm/pmm.h"
#include "lib/string.h"

KTEST_REGISTER(test_zram_oom, "ZRAM: pool exhaustion and reuse", KTEST_CAT_BOOT)
static void test_zram_oom(void)
{
	KTEST_BEGIN("ZRAM: pool exhaustion and reuse");

	zram_init();

	/* Clean up any slots left by prior tests */
	for (int i = 0; i < ZRAM_MAX_SLOTS; i++)
		zram_free_slot(i);

	/* Fill all ZRAM slots */
	int slots[ZRAM_MAX_SLOTS];
	uint8_t page[4096];
	int filled = 0;

	for (int i = 0; i < ZRAM_MAX_SLOTS; i++) {
		/* Use a simple pattern — different per slot for variety */
		memset(page, 0, sizeof(page));
		page[0] = (uint8_t)i;
		page[1] = (uint8_t)(i ^ 0xFF);

		slots[i] = zram_compress_page(page);
		if (slots[i] >= 0)
			filled++;
		else
			break;
	}
	KTEST_EQ(filled, ZRAM_MAX_SLOTS, "all ZRAM slots filled");

	/* Used slots should be at max */
	KTEST_EQ(zram_used_slots(), ZRAM_MAX_SLOTS, "used_slots == MAX");

	/* Next compress should fail gracefully (pool full) */
	memset(page, 0, sizeof(page));
	int overflow = zram_compress_page(page);
	KTEST_EQ(overflow, -1, "compress fails when pool full");

	/* Free one slot */
	zram_free_slot(slots[0]);
	KTEST_EQ(zram_used_slots(), ZRAM_MAX_SLOTS - 1, "used_slots after free");

	/* Should be able to compress again */
	memset(page, 0, sizeof(page));
	page[0] = 0xDE;
	int reuse = zram_compress_page(page);
	KTEST_GE(reuse, 0, "compress succeeds after freeing a slot");

	/* Verify the reused slot contains correct data */
	if (reuse >= 0) {
		uint8_t verify[4096];
		memset(verify, 0xFF, sizeof(verify));
		int ret = zram_decompress_page(reuse, verify);
		KTEST_EQ(ret, 0, "decompress reused slot OK");
		KTEST_EQ((int)verify[0], 0xDE, "reused slot data correct");
		zram_free_slot(reuse);
	}

	/* Cleanup remaining slots */
	for (int i = 1; i < ZRAM_MAX_SLOTS; i++) {
		if (slots[i] >= 0)
			zram_free_slot(slots[i]);
	}

	KTEST_EQ(zram_used_slots(), 0, "all slots freed");
}
