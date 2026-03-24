#include <dicron/sched.h>
#include <dicron/spinlock.h>
#include "lib/string.h"

/*
 * mlfq.c — Multi-Level Feedback Queue data structure.
 *
 * 4 priority levels, each a doubly-linked circular list.
 * Pure data structure — no context switching here.
 */

static struct {
	struct task *head;
	struct task *tail;
	int count;
} queues[MLFQ_LEVELS];

static spinlock_t mlfq_lock = SPINLOCK_INIT;

void mlfq_init(void)
{
	for (int i = 0; i < MLFQ_LEVELS; i++) {
		queues[i].head = NULL;
		queues[i].tail = NULL;
		queues[i].count = 0;
	}
}

void mlfq_enqueue(struct task *t)
{
	if (!t) return;
	int lvl = t->priority;
	if (lvl < 0) lvl = 0;
	if (lvl >= MLFQ_LEVELS) lvl = MLFQ_LEVELS - 1;
	t->priority = lvl;

	uint64_t flags = spin_lock_irqsave(&mlfq_lock);

	t->next = NULL;
	t->prev = queues[lvl].tail;
	if (queues[lvl].tail)
		queues[lvl].tail->next = t;
	else
		queues[lvl].head = t;
	queues[lvl].tail = t;
	queues[lvl].count++;

	spin_unlock_irqrestore(&mlfq_lock, flags);
}

void mlfq_remove(struct task *t)
{
	if (!t) return;
	int lvl = t->priority;
	if (lvl < 0 || lvl >= MLFQ_LEVELS) return;

	uint64_t flags = spin_lock_irqsave(&mlfq_lock);

	/* Check if the task is actually in this queue.
	   A task dequeued by pick_next has prev=NULL and next=NULL,
	   but is NOT the queue head. Removing it would corrupt the queue. */
	int in_queue = (t->prev != NULL) || (t->next != NULL) ||
		       (queues[lvl].head == t);

	if (!in_queue) {
		spin_unlock_irqrestore(&mlfq_lock, flags);
		return;
	}

	if (t->prev)
		t->prev->next = t->next;
	else
		queues[lvl].head = t->next;

	if (t->next)
		t->next->prev = t->prev;
	else
		queues[lvl].tail = t->prev;

	t->next = NULL;
	t->prev = NULL;
	queues[lvl].count--;

	spin_unlock_irqrestore(&mlfq_lock, flags);
}

void mlfq_demote(struct task *t)
{
	if (!t) return;
	mlfq_remove(t);
	if (t->priority < MLFQ_LEVELS - 1)
		t->priority++;
	t->remaining = mlfq_timeslice[t->priority];
	mlfq_enqueue(t);
}

void mlfq_boost_all(void)
{
	uint64_t flags = spin_lock_irqsave(&mlfq_lock);

	/* Move everything from levels 1..3 into level 0 */
	for (int lvl = 1; lvl < MLFQ_LEVELS; lvl++) {
		while (queues[lvl].head) {
			struct task *t = queues[lvl].head;

			/* unlink from current level */
			queues[lvl].head = t->next;
			if (queues[lvl].head)
				queues[lvl].head->prev = NULL;
			else
				queues[lvl].tail = NULL;
			queues[lvl].count--;

			/* set to level 0 and append */
			t->priority = 0;
			t->remaining = mlfq_timeslice[0];
			t->next = NULL;
			t->prev = queues[0].tail;
			if (queues[0].tail)
				queues[0].tail->next = t;
			else
				queues[0].head = t;
			queues[0].tail = t;
			queues[0].count++;
		}
	}

	spin_unlock_irqrestore(&mlfq_lock, flags);
}

struct task *mlfq_pick_next(void)
{
	uint64_t flags = spin_lock_irqsave(&mlfq_lock);

	struct task *picked = NULL;
	for (int lvl = 0; lvl < MLFQ_LEVELS; lvl++) {
		if (queues[lvl].head) {
			picked = queues[lvl].head;

			/* dequeue head */
			queues[lvl].head = picked->next;
			if (queues[lvl].head)
				queues[lvl].head->prev = NULL;
			else
				queues[lvl].tail = NULL;
			queues[lvl].count--;

			picked->next = NULL;
			picked->prev = NULL;
			break;
		}
	}

	spin_unlock_irqrestore(&mlfq_lock, flags);
	return picked;
}

int mlfq_queue_length(int level)
{
	if (level < 0 || level >= MLFQ_LEVELS) return 0;
	return queues[level].count;
}
