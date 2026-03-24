#include "ktest.h"
#include <dicron/sched.h>
#include <dicron/mem.h>
#include <dicron/io.h>
#include "mm/slab.h"
#include "mm/pmm.h"
#include "mm/vmm.h"
#include "mm/kheap.h"
#include "lib/string.h"

/*
 * Intentional error recovery tests for the memory subsystem.
 * Every test here deliberately triggers an error path that USED to panic.
 * If the kernel survives all of these, our graceful recovery works.
 */

static void errm_dummy(void *arg) { (void)arg; }

KTEST_REGISTER(ktest_error_memory, "memory error recovery", KTEST_CAT_BOOT)
static void ktest_error_memory(void)
{
	KTEST_BEGIN("memory error recovery");

	size_t baseline = pmm_free_pages_count();

	/* --- kmalloc edge cases --- */

	/* kmalloc(0) must return NULL, not crash */
	void *z = kmalloc(0);
	KTEST_NULL(z, "errm: kmalloc(0) returns NULL");

	/* kmalloc(huge) must return NULL, not OOM-panic */
	void *huge = kmalloc(1024ULL * 1024ULL * 1024ULL);
	KTEST_NULL(huge, "errm: kmalloc(1GB) returns NULL");

	/* kfree(NULL) must be safe (no-op) */
	kfree(NULL);
	KTEST_TRUE(1, "errm: kfree(NULL) survived");

	/* krealloc(NULL, size) must behave as kmalloc */
	void *r1 = krealloc(NULL, 64);
	KTEST_NOT_NULL(r1, "errm: krealloc(NULL,64) works as kmalloc");
	kfree(r1);

	/* krealloc(ptr, 0) must behave as kfree */
	void *r2 = kmalloc(64);
	void *r3 = krealloc(r2, 0);
	KTEST_NULL(r3, "errm: krealloc(ptr,0) works as kfree");

	/* --- slab cache edge cases --- */

	/* kmem_cache_alloc(NULL) must return NULL, not panic */
	void *sn = kmem_cache_alloc(NULL);
	KTEST_NULL(sn, "errm: kmem_cache_alloc(NULL) returns NULL");

	/* kmem_cache_free(NULL, ptr) must survive */
	kmem_cache_free(NULL, (void *)0x1234);
	KTEST_TRUE(1, "errm: kmem_cache_free(NULL, ptr) survived");

	/* kmem_cache_free(cache, NULL) must survive */
	struct kmem_cache *tc = kmem_cache_create("err-test", 32, NULL, NULL);
	KTEST_NOT_NULL(tc, "errm: create test cache");
	kmem_cache_free(tc, NULL);
	KTEST_TRUE(1, "errm: kmem_cache_free(cache, NULL) survived");

	/* Double free on slab: allocator should detect and survive */
	void *dbl = kmem_cache_alloc(tc);
	KTEST_NOT_NULL(dbl, "errm: alloc for double-free test");
	kmem_cache_free(tc, dbl);
	kmem_cache_free(tc, dbl);  /* second free — should log, not crash */
	KTEST_TRUE(1, "errm: slab double-free survived");

	/* Allocate until the slab can't grow (PMM exhaustion simulation) —
	   just verify it returns NULL eventually, not panics */
	int alloc_count = 0;
	void *ptrs[2000];
	for (int i = 0; i < 2000; i++) {
		ptrs[i] = kmem_cache_alloc(tc);
		if (!ptrs[i]) break;
		alloc_count++;
	}
	KTEST_GT(alloc_count, 0, "errm: slab allocated some before saturation");
	/* Free them all back */
	for (int i = 0; i < alloc_count; i++)
		kmem_cache_free(tc, ptrs[i]);
	KTEST_TRUE(1, "errm: mass slab alloc/free cycle survived");

	/* --- kstack edge cases --- */

	/* kstack_alloc(0) — edge case, should still work or return NULL */
	void *ks0 = kstack_alloc(0);
	/* Either NULL or valid is acceptable — just don't crash */
	if (ks0) kstack_free(ks0, 0);
	KTEST_TRUE(1, "errm: kstack_alloc(0) survived");

	/* kstack_alloc(33) — exceeds 32-page limit, must return NULL */
	void *ks_huge = kstack_alloc(33);
	KTEST_NULL(ks_huge, "errm: kstack_alloc(33) returns NULL");

	/* kstack_free(NULL) — must be safe */
	kstack_free(NULL, 4);
	KTEST_TRUE(1, "errm: kstack_free(NULL) survived");

	/* --- kthread edge cases --- */

	/* kthread_create(NULL, arg) must return NULL */
	struct task *bad1 = kthread_create(NULL, (void *)0xDEAD);
	KTEST_NULL(bad1, "errm: kthread_create(NULL fn) returns NULL");

	/* kthread_free(NULL) must be safe */
	kthread_free(NULL);
	KTEST_TRUE(1, "errm: kthread_free(NULL) survived");

	/* Create and immediately free without running — tests cleanup path */
	struct task *t = kthread_create(errm_dummy, NULL);
	KTEST_NOT_NULL(t, "errm: create for immediate free");
	kthread_free(t);
	KTEST_TRUE(1, "errm: create-then-free without running survived");

	/* Verify no pages leaked from all error recovery paths */
	size_t final = pmm_free_pages_count();
	/* Error tests create slab caches and mass allocs that keep empty slabs warm */
	size_t delta = baseline - final;
	KTEST_LT(delta, 32, "errm: error recovery leaked < 32 pages overhead");

	kio_printf("  [ERRM] Memory error recovery: all %d cases survived\n",
		   ktest_stats.total);
}
