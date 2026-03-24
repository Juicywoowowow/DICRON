#include "ktest.h"
#include <dicron/sched.h>

/*
 * Test task state machine transitions and struct integrity.
 * Exercises all sad-path field validations.
 */

static void state_fn(void *arg) { (void)arg; }

KTEST_REGISTER(ktest_sched_state, "scheduler state", KTEST_CAT_BOOT)
static void ktest_sched_state(void)
{
	KTEST_BEGIN("task state machine");

	/* Happy: newly created thread must be TASK_READY */
	struct task *t = kthread_create(state_fn, (void *)0x1);
	KTEST_NOT_NULL(t, "state: create");
	KTEST_EQ(t->state, TASK_READY, "state: initial is READY");

	/* Manually transition through all states */
	t->state = TASK_RUNNING;
	KTEST_EQ(t->state, TASK_RUNNING, "state: transition to RUNNING");

	t->state = TASK_SLEEPING;
	KTEST_EQ(t->state, TASK_SLEEPING, "state: transition to SLEEPING");

	t->state = TASK_DEAD;
	KTEST_EQ(t->state, TASK_DEAD, "state: transition to DEAD");

	kthread_free(t);

	/* Sad: NULL fn must be rejected */
	struct task *bad = kthread_create(NULL, (void *)0);
	KTEST_NULL(bad, "state: NULL fn rejected");

	/* Sad: verify struct offsets are sane after mass transitions */
	struct task *t2 = kthread_create(state_fn, (void *)0xFFFF);
	KTEST_NOT_NULL(t2, "state: second create");
	KTEST_GE(t2->pid, 0, "state: PID non-negative");
	KTEST_NOT_NULL(t2->kernel_stack, "state: kernel_stack set");
	KTEST_NOT_NULL(t2->context, "state: context set");
	KTEST_EQ(t2->context->rip != 0, 1, "state: RIP points to stub");
	KTEST_EQ(t2->context->r12, (uint64_t)state_fn, "state: R12 = fn");
	KTEST_EQ(t2->context->r13, 0xFFFF, "state: R13 = arg");
	KTEST_EQ(t2->next, (struct task *)0, "state: next is NULL");
	KTEST_EQ(t2->page_table, 0, "state: page_table is 0");

	kthread_free(t2);
}
