#include "ktest.h"
#include <dicron/sched.h>

/*
 * Test MLFQ demotion: a task that exhausts its timeslice
 * should be moved to the next lower priority level.
 */

static void demote_dummy(void *arg) { (void)arg; }

KTEST_REGISTER(ktest_mlfq_demote, "MLFQ demote", KTEST_CAT_BOOT)
static void ktest_mlfq_demote(void)
{
	KTEST_BEGIN("MLFQ demotion");

	struct task *t = kthread_create(demote_dummy, NULL);
	KTEST_NOT_NULL(t, "demote: create");
	KTEST_EQ(t->priority, 0, "demote: starts at level 0");

	mlfq_enqueue(t);

	/* Demote once */
	mlfq_demote(t);
	KTEST_EQ(t->priority, 1, "demote: now at level 1");
	KTEST_EQ(t->remaining, mlfq_timeslice[1], "demote: timeslice reset to level 1");

	/* Demote again */
	mlfq_demote(t);
	KTEST_EQ(t->priority, 2, "demote: now at level 2");

	/* Demote to bottom */
	mlfq_demote(t);
	KTEST_EQ(t->priority, 3, "demote: now at level 3 (bottom)");

	/* Demote at bottom should stay at bottom */
	mlfq_demote(t);
	KTEST_EQ(t->priority, 3, "demote: stays at level 3");

	mlfq_remove(t);
	kthread_free(t);
}
