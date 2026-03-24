#include <dicron/sched.h>
#include <dicron/spinlock.h>

/*
 * waitqueue.c — Blocking wait queues for I/O and synchronization.
 *
 * kwait(): removes current task from MLFQ, marks SLEEPING, appends to wq.
 * kwake(): moves first sleeping task back to MLFQ at its current priority.
 */

void kwait(struct wait_queue *wq)
{
	struct task *cur = sched_current();
	if (!cur) return;

	/* Remove from run queue and mark sleeping */
	cur->state = TASK_SLEEPING;
	mlfq_remove(cur);

	/* Append to wait queue tail */
	cur->next = NULL;
	cur->prev = wq->tail;
	if (wq->tail)
		wq->tail->next = cur;
	else
		wq->head = cur;
	wq->tail = cur;

	/* Yield to next ready task */
	schedule();
}

void kwake(struct wait_queue *wq)
{
	if (!wq->head) return;

	struct task *t = wq->head;
	wq->head = t->next;
	if (wq->head)
		wq->head->prev = NULL;
	else
		wq->tail = NULL;

	t->next = NULL;
	t->prev = NULL;
	t->state = TASK_READY;
	t->remaining = mlfq_timeslice[t->priority];
	mlfq_enqueue(t);
}

int kwake_all(struct wait_queue *wq)
{
	int woken = 0;
	while (wq->head) {
		kwake(wq);
		woken++;
	}
	return woken;
}
