/*
 * test_swap_reclaim.c — Register page, trigger reclaim, verify frame freed
 */

#include "ktest.h"
#include "mm/swap.h"
#include "mm/zram.h"
#include "mm/pmm.h"
#include "lib/string.h"
#include <dicron/errno.h>

KTEST_REGISTER(test_swap_reclaim, "Swap: cooperative reclaim", KTEST_CAT_BOOT)
static void test_swap_reclaim(void)
{
	KTEST_BEGIN("Swap: cooperative reclaim");

	zram_init();

	/* Allocate a page and fill with a pattern */
	void *page = pmm_alloc_page();
	KTEST_NOT_NULL(page, "page allocated for reclaim test");
	if (!page)
		return;

	memset(page, 0, PAGE_SIZE);	/* mostly-zero = highly compressible */

	/* Register it as reclaimable */
	int ret = swap_register(page);
	KTEST_EQ(ret, 0, "page registered for reclaim");

	/* Record free count before reclaim */
	size_t free_before = pmm_free_pages_count();

	/* Trigger reclaim — should compress via ZRAM and free the frame */
	void *reclaimed = swap_try_reclaim(0);
	KTEST_NOT_NULL(reclaimed, "reclaim returned a page");

	/* Free count should have increased (frame returned to PMM) */
	size_t free_after = pmm_free_pages_count();
	KTEST_GT((long long)free_after, (long long)free_before,
		 "PMM free count increased after reclaim");

	/* Unregister should fail since page was already evicted */
	ret = swap_unregister(page);
	KTEST_EQ(ret, -ENOENT, "unregister after eviction returns ENOENT");
}
