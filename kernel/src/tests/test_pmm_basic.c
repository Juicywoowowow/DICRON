#include "ktest.h"
#include "mm/pmm.h"

KTEST_REGISTER(ktest_pmm_basic, "PMM basic alloc/free", KTEST_CAT_BOOT)
static void ktest_pmm_basic(void)
{
	KTEST_BEGIN("PMM basic alloc/free");

	size_t before = pmm_free_pages_count();

	void *p = pmm_alloc_page();
	KTEST_NOT_NULL(p, "alloc single page");
	KTEST_EQ(pmm_free_pages_count(), before - 1, "free count decremented");

	pmm_free_page(p);
	KTEST_EQ(pmm_free_pages_count(), before, "free count restored");

	KTEST_GT(pmm_total_pages_count(), 0, "total pages > 0");
	KTEST_GE(pmm_total_pages_count(), pmm_free_pages_count(),
		 "total >= free");
}
