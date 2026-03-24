#include "ktest.h"
#include <dicron/sched.h>
#include <dicron/io.h>
#include <dicron/time.h>
#include "mm/pmm.h"
#include "mm/kheap.h"
#include "lib/string.h"

/*
 * Intentional error recovery tests for the scheduler and MLFQ.
 * Pushes the MLFQ data structure with bad inputs to verify
 * the kernel doesn't crash on corrupted or edge-case state.
 */

static void errs_dummy(void *arg) { (void)arg; }

KTEST_REGISTER(ktest_error_sched, "scheduler error recovery", KTEST_CAT_BOOT)
static void ktest_error_sched(void)
{
	KTEST_BEGIN("scheduler error recovery");

	/* --- MLFQ NULL safety --- */

	/* mlfq_enqueue(NULL) must not crash */
	mlfq_enqueue(NULL);
	KTEST_TRUE(1, "errs: mlfq_enqueue(NULL) survived");

	/* mlfq_remove(NULL) must not crash */
	mlfq_remove(NULL);
	KTEST_TRUE(1, "errs: mlfq_remove(NULL) survived");

	/* mlfq_demote(NULL) must not crash */
	mlfq_demote(NULL);
	KTEST_TRUE(1, "errs: mlfq_demote(NULL) survived");

	/* mlfq_pick_next on empty queues must return NULL */
	/* First drain any leftover state */
	while (mlfq_pick_next()) {}
	struct task *empty = mlfq_pick_next();
	KTEST_NULL(empty, "errs: pick_next on empty returns NULL");

	/* --- MLFQ out-of-bounds priority --- */

	struct task *t1 = kthread_create(errs_dummy, NULL);
	KTEST_NOT_NULL(t1, "errs: create for OOB priority");

	/* Negative priority should be clamped to 0 */
	t1->priority = -5;
	mlfq_enqueue(t1);
	KTEST_EQ(t1->priority, 0, "errs: negative priority clamped to 0");
	mlfq_remove(t1);

	/* Priority above max should be clamped to MLFQ_LEVELS-1 */
	t1->priority = 999;
	mlfq_enqueue(t1);
	KTEST_EQ(t1->priority, MLFQ_LEVELS - 1, "errs: huge priority clamped to max");
	mlfq_remove(t1);
	kthread_free(t1);

	/* --- MLFQ double remove --- */

	struct task *t2 = kthread_create(errs_dummy, NULL);
	KTEST_NOT_NULL(t2, "errs: create for double remove");
	t2->priority = 0;
	mlfq_enqueue(t2);
	mlfq_remove(t2);
	/* Second remove — list pointers are NULL, should not crash */
	mlfq_remove(t2);
	KTEST_TRUE(1, "errs: double mlfq_remove survived");
	kthread_free(t2);

	/* --- MLFQ demote past bottom --- */

	struct task *t3 = kthread_create(errs_dummy, NULL);
	KTEST_NOT_NULL(t3, "errs: create for demote-past-bottom");
	t3->priority = MLFQ_LEVELS - 1;
	mlfq_enqueue(t3);
	mlfq_demote(t3); /* already at bottom — should stay */
	KTEST_EQ(t3->priority, MLFQ_LEVELS - 1, "errs: demote at bottom stays");
	mlfq_remove(t3);
	kthread_free(t3);

	/* --- MLFQ boost on empty queues --- */

	mlfq_boost_all();
	KTEST_TRUE(1, "errs: boost_all on empty queues survived");

	/* --- MLFQ queue_length out of bounds --- */

	int neg_len = mlfq_queue_length(-1);
	KTEST_EQ(neg_len, 0, "errs: queue_length(-1) returns 0");
	int huge_len = mlfq_queue_length(9999);
	KTEST_EQ(huge_len, 0, "errs: queue_length(9999) returns 0");

	/* --- MLFQ rapid enqueue/remove interleaving --- */

	struct task *burst[50];
	for (int i = 0; i < 50; i++) {
		burst[i] = kthread_create(errs_dummy, NULL);
		KTEST_NOT_NULL(burst[i], "errs: burst create");
		burst[i]->priority = i % MLFQ_LEVELS;
	}
	/* Enqueue all */
	for (int i = 0; i < 50; i++)
		mlfq_enqueue(burst[i]);
	/* Remove every other one */
	for (int i = 0; i < 50; i += 2)
		mlfq_remove(burst[i]);
	/* Re-enqueue the removed ones at different priorities */
	for (int i = 0; i < 50; i += 2) {
		burst[i]->priority = (i + 1) % MLFQ_LEVELS;
		mlfq_enqueue(burst[i]);
	}
	/* Demote all */
	for (int i = 0; i < 50; i++)
		mlfq_demote(burst[i]);
	/* Boost all back */
	mlfq_boost_all();
	/* Verify all at level 0 */
	int all_boosted = 1;
	for (int i = 0; i < 50; i++) {
		if (burst[i]->priority != 0) all_boosted = 0;
	}
	KTEST_TRUE(all_boosted, "errs: burst enqueue/remove/demote/boost all at level 0");
	/* Drain and free */
	for (int i = 0; i < 50; i++)
		mlfq_remove(burst[i]);
	for (int i = 0; i < 50; i++)
		kthread_free(burst[i]);

	/* --- wait queue edge cases --- */

	/* kwake on empty wait queue — should not crash */
	struct wait_queue empty_wq = WAIT_QUEUE_INIT;
	kwake(&empty_wq);
	KTEST_TRUE(1, "errs: kwake on empty wait queue survived");

	/* kwake_all on empty wait queue */
	int woken = kwake_all(&empty_wq);
	KTEST_EQ(woken, 0, "errs: kwake_all on empty returns 0");

	/* --- task state corruption resilience --- */

	struct task *t4 = kthread_create(errs_dummy, NULL);
	KTEST_NOT_NULL(t4, "errs: create for state corruption test");

	/* Set nonsense state values — the kernel should not crash */
	t4->state = (enum task_state)255;
	KTEST_EQ((int)t4->state, 255, "errs: task accepts arbitrary state value");

	/* Set state back to valid and verify it's usable */
	t4->state = TASK_READY;
	t4->priority = 0;
	mlfq_enqueue(t4);
	struct task *picked = mlfq_pick_next();
	KTEST_EQ(picked, t4, "errs: recovered task still works in MLFQ");
	kthread_free(t4);

	/* --- preempt edge cases --- */

	/* preempt functions when scheduler not active should be safe */
	preempt_clear();
	KTEST_EQ(preempt_need_reschedule(), 0, "errs: cleared reschedule flag");

	kio_printf("  [ERRS] Scheduler error recovery: all %d cases survived\n",
		   ktest_stats.total);
}
