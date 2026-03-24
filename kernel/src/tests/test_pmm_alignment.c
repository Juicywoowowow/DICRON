#include "ktest.h"
#include "mm/pmm.h"

KTEST_REGISTER(ktest_pmm_alignment, "PMM alignment guarantees", KTEST_CAT_BOOT)
static void ktest_pmm_alignment(void)
{
	KTEST_BEGIN("PMM alignment guarantees");

	void *p0 = pmm_alloc_pages(0);
	void *p1 = pmm_alloc_pages(1);
	void *p2 = pmm_alloc_pages(2);
	void *p4 = pmm_alloc_pages(4);

	KTEST_EQ((uint64_t)p0 % (4096), 0, "order 0 page-aligned");
	KTEST_EQ((uint64_t)p1 % (4096 * 2), 0, "order 1 2-page-aligned");
	KTEST_EQ((uint64_t)p2 % (4096 * 4), 0, "order 2 4-page-aligned");
	KTEST_EQ((uint64_t)p4 % (4096 * 16), 0, "order 4 16-page-aligned");

	pmm_free_pages(p0, 0);
	pmm_free_pages(p1, 1);
	pmm_free_pages(p2, 2);
	pmm_free_pages(p4, 4);
}
