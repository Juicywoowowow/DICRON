#include "ktest.h"
#include <dicron/sched.h>
#include "mm/pmm.h"
#include "mm/kheap.h"

/*
 * Stress test: mass create/free with strict zero-leak verification.
 * Warms the full batch first so all PT pages are pre-allocated, then
 * measures that the second cycle is perfectly zero-leak.
 */

static void dummy_fn(void *arg) { (void)arg; }

#define STRESS_BATCH 200

KTEST_REGISTER(ktest_sched_stress, "scheduler stress", KTEST_CAT_BOOT)
static void ktest_sched_stress(void)
{
	KTEST_BEGIN("scheduler stress create/free");

	struct task *threads[STRESS_BATCH];

	/* Warm up: allocate the full batch to pre-populate all VMM page tables */
	int warmed = 0;
	for (int i = 0; i < STRESS_BATCH; i++) {
		threads[i] = kthread_create(dummy_fn, (void *)(uint64_t)i);
		if (!threads[i]) break;
		warmed++;
	}
	KTEST_EQ(warmed, STRESS_BATCH, "stress: warmup created all threads");
	for (int i = 0; i < warmed; i++)
		kthread_free(threads[i]);

	/* Measured batch: all virtual slots are recycled, delta must be 0 */
	size_t before = pmm_free_pages_count();
	int created = 0;
	for (int i = 0; i < STRESS_BATCH; i++) {
		threads[i] = kthread_create(dummy_fn, (void *)(uint64_t)i);
		if (!threads[i]) break;
		created++;
	}
	KTEST_EQ(created, STRESS_BATCH, "stress: created all 200 threads");
	for (int i = 0; i < created; i++)
		kthread_free(threads[i]);

	size_t after = pmm_free_pages_count();
	KTEST_EQ(after, before, "stress: zero page leak after create/free cycle");

	/* Second batch — must also be zero leak */
	size_t before2 = pmm_free_pages_count();
	for (int i = 0; i < STRESS_BATCH; i++)
		threads[i] = kthread_create(dummy_fn, (void *)(uint64_t)i);
	for (int i = 0; i < STRESS_BATCH; i++)
		kthread_free(threads[i]);
	size_t after2 = pmm_free_pages_count();
	KTEST_EQ(after2, before2, "stress: second batch also zero leak");
}
