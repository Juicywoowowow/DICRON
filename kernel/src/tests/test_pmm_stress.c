#include "ktest.h"
#include "mm/pmm.h"

KTEST_REGISTER(ktest_pmm_stress, "PMM stress", KTEST_CAT_BOOT)
static void ktest_pmm_stress(void)
{
	KTEST_BEGIN("PMM stress (250 mixed-order allocs)");

	size_t start_free = pmm_free_pages_count();

	void *pages[250];
	int ok = 1;
	for (int i = 0; i < 250; i++) {
		unsigned int order = (unsigned int)(i % 3);
		pages[i] = pmm_alloc_pages(order);
		if (!pages[i]) { ok = 0; break; }
	}
	KTEST_TRUE(ok, "250 allocs succeeded");

	/* out-of-order free: evens then odds */
	for (int i = 0; i < 250; i += 2)
		pmm_free_pages(pages[i], (unsigned int)(i % 3));
	for (int i = 1; i < 250; i += 2)
		pmm_free_pages(pages[i], (unsigned int)(i % 3));

	KTEST_EQ(pmm_free_pages_count(), start_free,
		 "all pages returned after stress");

	/* order exhaustion */
	void *huge[512];
	int count = 0;
	for (int i = 0; i < 512; i++) {
		huge[i] = pmm_alloc_pages(MAX_ORDER);
		if (!huge[i]) break;
		count++;
	}
	void *impossible = pmm_alloc_pages(MAX_ORDER);
	KTEST_NULL(impossible, "OOM returns NULL at max order");

	if (impossible) {
		pmm_free_pages(impossible, MAX_ORDER);
	}

	for (int i = 0; i < count; i++)
		pmm_free_pages(huge[i], MAX_ORDER);
}
