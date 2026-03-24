#include "ktest.h"
#include <dicron/sched.h>
#include "mm/pmm.h"
#include "mm/vmm.h"

/*
 * Verify kstack virtual address recycling.
 * After freeing a stack, the next alloc should reuse the same
 * virtual range, preventing page table proliferation.
 */

KTEST_REGISTER(ktest_kstack_recycle, "kstack recycle", KTEST_CAT_BOOT)
static void ktest_kstack_recycle(void)
{
	KTEST_BEGIN("kstack virtual address recycling");

	/* Allocate and free a stack, then allocate again */
	void *s1 = kstack_alloc(4);
	KTEST_NOT_NULL(s1, "recycle: first alloc");

	kstack_free(s1, 4);

	void *s2 = kstack_alloc(4);
	KTEST_NOT_NULL(s2, "recycle: second alloc");

	/* s2 should reuse the same virtual slot as s1 */
	KTEST_EQ((uint64_t)s1, (uint64_t)s2,
		 "recycle: second alloc reused first's virtual range");

	kstack_free(s2, 4);

	/* Verify that physical pages are correctly mapped after reuse */
	void *s3 = kstack_alloc(4);
	KTEST_NOT_NULL(s3, "recycle: third alloc");

	uint64_t phys = vmm_virt_to_phys((uint64_t)s3 - 1);
	KTEST_NE(phys, 0, "recycle: reused slot has valid physical mapping");

	kstack_free(s3, 4);

	/* Multiple frees then allocs should LIFO recycle */
	void *a = kstack_alloc(4);
	void *b = kstack_alloc(4);
	void *c = kstack_alloc(4);
	kstack_free(c, 4);
	kstack_free(b, 4);
	kstack_free(a, 4);

	void *a2 = kstack_alloc(4);
	void *b2 = kstack_alloc(4);
	void *c2 = kstack_alloc(4);

	/* LIFO: a2 gets a's slot, b2 gets b's, c2 gets c's */
	KTEST_EQ((uint64_t)a2, (uint64_t)a, "recycle: LIFO slot A");
	KTEST_EQ((uint64_t)b2, (uint64_t)b, "recycle: LIFO slot B");
	KTEST_EQ((uint64_t)c2, (uint64_t)c, "recycle: LIFO slot C");

	kstack_free(a2, 4);
	kstack_free(b2, 4);
	kstack_free(c2, 4);
}
