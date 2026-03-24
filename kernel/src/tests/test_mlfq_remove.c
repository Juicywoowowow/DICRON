#include "ktest.h"
#include <dicron/sched.h>
#include <dicron/time.h>
#include <dicron/io.h>

/*
 * Test MLFQ remove from middle of queue (edge case for doubly-linked list).
 * Also tests queue_length correctness.
 */

static void remove_dummy(void *arg) { (void)arg; }

KTEST_REGISTER(ktest_mlfq_remove_mid, "MLFQ remove", KTEST_CAT_BOOT)
static void ktest_mlfq_remove_mid(void)
{
	KTEST_BEGIN("MLFQ remove from middle");

	struct task *a = kthread_create(remove_dummy, NULL);
	struct task *b = kthread_create(remove_dummy, NULL);
	struct task *c = kthread_create(remove_dummy, NULL);
	struct task *d = kthread_create(remove_dummy, NULL);
	KTEST_NOT_NULL(a, "rm_mid: create a");
	KTEST_NOT_NULL(b, "rm_mid: create b");
	KTEST_NOT_NULL(c, "rm_mid: create c");
	KTEST_NOT_NULL(d, "rm_mid: create d");

	a->priority = 1;
	b->priority = 1;
	c->priority = 1;
	d->priority = 1;

	mlfq_enqueue(a);
	mlfq_enqueue(b);
	mlfq_enqueue(c);
	mlfq_enqueue(d);

	KTEST_EQ(mlfq_queue_length(1), 4, "rm_mid: length is 4");

	/* Remove from middle (b) */
	mlfq_remove(b);
	KTEST_EQ(mlfq_queue_length(1), 3, "rm_mid: length is 3 after remove b");

	/* Remove head (a) */
	mlfq_remove(a);
	KTEST_EQ(mlfq_queue_length(1), 2, "rm_mid: length is 2 after remove a");

	/* Remove tail (d) */
	mlfq_remove(d);
	KTEST_EQ(mlfq_queue_length(1), 1, "rm_mid: length is 1 after remove d");

	/* Only c should be left */
	struct task *last = mlfq_pick_next();
	KTEST_EQ(last, c, "rm_mid: c is the only remaining task");

	/* Queue should be empty now */
	KTEST_EQ(mlfq_queue_length(1), 0, "rm_mid: queue empty");

	kthread_free(a);
	kthread_free(b);
	kthread_free(c);
	kthread_free(d);
}
