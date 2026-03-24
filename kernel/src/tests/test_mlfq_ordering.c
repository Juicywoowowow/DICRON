#include "ktest.h"
#include <dicron/sched.h>
#include <dicron/time.h>

/*
 * Test MLFQ enqueue/dequeue ordering and pick_next priority behavior.
 */

static void mlfq_dummy(void *arg) { (void)arg; }

KTEST_REGISTER(ktest_mlfq_ordering, "MLFQ ordering", KTEST_CAT_BOOT)
static void ktest_mlfq_ordering(void)
{
	KTEST_BEGIN("MLFQ queue ordering");

	struct task *hi = kthread_create(mlfq_dummy, NULL);
	struct task *lo = kthread_create(mlfq_dummy, NULL);
	KTEST_NOT_NULL(hi, "mlfq: create hi");
	KTEST_NOT_NULL(lo, "mlfq: create lo");

	hi->priority = 0;
	lo->priority = 2;

	/* Enqueue low first, high second */
	mlfq_enqueue(lo);
	mlfq_enqueue(hi);

	/* pick_next should return the higher priority task */
	struct task *picked = mlfq_pick_next();
	KTEST_EQ(picked, hi, "mlfq: pick_next returns highest priority");

	/* Next pick should return lo */
	struct task *picked2 = mlfq_pick_next();
	KTEST_EQ(picked2, lo, "mlfq: second pick returns lower priority");

	/* Empty queues → NULL */
	struct task *empty = mlfq_pick_next();
	KTEST_NULL(empty, "mlfq: empty queue returns NULL");

	kthread_free(hi);
	kthread_free(lo);
}
