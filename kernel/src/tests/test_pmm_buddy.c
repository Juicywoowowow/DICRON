#include "ktest.h"
#include "mm/pmm.h"

KTEST_REGISTER(ktest_pmm_buddy, "PMM buddy orders", KTEST_CAT_BOOT)
static void ktest_pmm_buddy(void)
{
	KTEST_BEGIN("PMM buddy orders");

	size_t before = pmm_free_pages_count();

	void *p0 = pmm_alloc_pages(0);
	void *p1 = pmm_alloc_pages(1);
	void *p2 = pmm_alloc_pages(2);
	void *p4 = pmm_alloc_pages(4);

	KTEST_NOT_NULL(p0, "alloc order 0");
	KTEST_NOT_NULL(p1, "alloc order 1");
	KTEST_NOT_NULL(p2, "alloc order 2");
	KTEST_NOT_NULL(p4, "alloc order 4");

	size_t expected = 1 + 2 + 4 + 16;
	KTEST_EQ(before - pmm_free_pages_count(), expected,
		 "page count matches orders");

	pmm_free_pages(p4, 4);
	pmm_free_pages(p2, 2);
	pmm_free_pages(p1, 1);
	pmm_free_page(p0);

	KTEST_EQ(pmm_free_pages_count(), before, "coalesce restores count");
}
