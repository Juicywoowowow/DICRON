#include "ktest.h"
#include "mm/pmm.h"

KTEST_REGISTER(ktest_pmm_double_free, "PMM double free defense", KTEST_CAT_BOOT)
static void ktest_pmm_double_free(void)
{
	KTEST_BEGIN("PMM double free defense");

	size_t before = pmm_free_pages_count();

	void *p = pmm_alloc_pages(0);
	pmm_free_page(p);
	size_t after_first = pmm_free_pages_count();

	/* second free — should be caught and not corrupt the allocator */
	pmm_free_page(p);
	size_t after_second = pmm_free_pages_count();

	KTEST_EQ(after_first, before, "first free restores count");
	KTEST_EQ(after_second, before, "double free does not inflate count");

	/* allocator still works after double free attempt */
	void *p2 = pmm_alloc_page();
	KTEST_NOT_NULL(p2, "allocator functional after double free");
	pmm_free_page(p2);
}
