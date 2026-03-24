#include <dicron/sched.h>
#include <dicron/spinlock.h>
#include <dicron/log.h>
#include <dicron/panic.h>
#include <dicron/mem.h>
#include "lib/string.h"

/*
 * sched.c — Scheduler core.
 *
 * Manages the 'current' task pointer, the idle task, and the
 * schedule() context switch driven by MLFQ pick_next().
 */

static struct task *current_task = NULL;
static struct task *idle_task = NULL;
static spinlock_t sched_lock = SPINLOCK_INIT;
static int sched_active = 0;

static void idle_fn(void *arg)
{
	(void)arg;
	for (;;)
		__asm__ volatile("sti; hlt");
}

struct task *sched_current(void)
{
	return current_task;
}

void sched_init(void)
{
	mlfq_init();
	preempt_init();

	/* Create the idle task — it never enters the MLFQ.
	   It's the fallback when all queues are empty. */
	idle_task = kthread_create(idle_fn, NULL);
	if (!idle_task) kpanic("sched_init: failed to create idle task");
	idle_task->pid = 0;
	idle_task->priority = MLFQ_LEVELS; /* below all real levels */

	/* The boot context becomes the "init" task (PID 1).
	   We fabricate a task struct for it so the scheduler can track it. */
	struct task *init = kmalloc(sizeof(struct task));
	if (!init) kpanic("sched_init: failed to create init task");
	memset(init, 0, sizeof(*init));
	init->pid = -1; /* boot context — not a real process */
	init->state = TASK_RUNNING;
	init->priority = 0;
	init->remaining = mlfq_timeslice[0];
	/* The boot context is already running on a valid stack (Limine's boot
	 * stack).  We don't allocate a new one, but switch_context will save
	 * the current RSP into init->context, so it works correctly. */

	current_task = init;
	sched_active = 1;

	klog(KLOG_INFO, "Scheduler initialized (MLFQ, %d levels)\n", MLFQ_LEVELS);
}

void schedule(void)
{
	if (!sched_active) return;

	uint64_t flags = spin_lock_irqsave(&sched_lock);

	struct task *prev = current_task;
	struct task *next = mlfq_pick_next();

	if (!next) {
		/* All queues empty → run idle task */
		next = idle_task;
	}

	if (next == prev) {
		/* Same task, just reset timeslice and continue */
		prev->remaining = mlfq_timeslice[prev->priority < MLFQ_LEVELS ?
						 prev->priority : 0];
		spin_unlock_irqrestore(&sched_lock, flags);
		return;
	}

	/* Put previous task back on the MLFQ if it's still runnable */
	if (prev->state == TASK_RUNNING) {
		/* If timeslice was exhausted, demote first */
		if (prev->remaining <= 0 && prev != idle_task) {
			if (prev->priority < MLFQ_LEVELS - 1)
				prev->priority++;
			prev->remaining = mlfq_timeslice[prev->priority];
		}
		prev->state = TASK_READY;
		if (prev != idle_task)
			mlfq_enqueue(prev);
	}

	next->state = TASK_RUNNING;
	next->remaining = mlfq_timeslice[next->priority < MLFQ_LEVELS ?
					 next->priority : 0];
	current_task = next;

	preempt_clear();

	switch_context(&prev->context, next->context);

	spin_unlock_irqrestore(&sched_lock, flags);
}

void sched_release_lock(void)
{
	spin_unlock(&sched_lock);
}
