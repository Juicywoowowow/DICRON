#include "ktest.h"
#include <dicron/sched.h>

/*
 * Test MLFQ priority boost: all tasks should return to level 0.
 */

static void boost_dummy(void *arg) { (void)arg; }

KTEST_REGISTER(ktest_mlfq_boost, "MLFQ boost", KTEST_CAT_BOOT)
static void ktest_mlfq_boost(void)
{
	KTEST_BEGIN("MLFQ priority boost");

	struct task *a = kthread_create(boost_dummy, NULL);
	struct task *b = kthread_create(boost_dummy, NULL);
	struct task *c = kthread_create(boost_dummy, NULL);
	KTEST_NOT_NULL(a, "boost: create a");
	KTEST_NOT_NULL(b, "boost: create b");
	KTEST_NOT_NULL(c, "boost: create c");

	/* Scatter across levels */
	a->priority = 0;
	b->priority = 2;
	c->priority = 3;

	mlfq_enqueue(a);
	mlfq_enqueue(b);
	mlfq_enqueue(c);

	/* Boost all */
	mlfq_boost_all();

	/* All three should now be at level 0 */
	KTEST_EQ(a->priority, 0, "boost: a stays at 0");
	KTEST_EQ(b->priority, 0, "boost: b boosted to 0");
	KTEST_EQ(c->priority, 0, "boost: c boosted to 0");

	/* Verify timeslices were reset */
	KTEST_EQ(b->remaining, mlfq_timeslice[0], "boost: b timeslice reset");
	KTEST_EQ(c->remaining, mlfq_timeslice[0], "boost: c timeslice reset");

	/* Queue length at level 0 should be 3 */
	KTEST_EQ(mlfq_queue_length(0), 3, "boost: all 3 in level 0");
	KTEST_EQ(mlfq_queue_length(2), 0, "boost: level 2 empty");
	KTEST_EQ(mlfq_queue_length(3), 0, "boost: level 3 empty");

	/* Drain and free */
	mlfq_remove(a);
	mlfq_remove(b);
	mlfq_remove(c);
	kthread_free(a);
	kthread_free(b);
	kthread_free(c);
}
