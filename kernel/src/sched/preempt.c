#include <dicron/sched.h>
#include <dicron/spinlock.h>
#include <dicron/log.h>
#include <dicron/panic.h>

/*
 * preempt.c — Timer-driven preemption for MLFQ.
 *
 * Called from the PIT IRQ handler each tick (1ms).
 * Decrements the running task's remaining budget, triggers demotion,
 * manages the priority boost timer, and sets a reschedule flag.
 */

static volatile int need_resched = 0;
static volatile uint64_t boost_counter = 0;

void preempt_init(void)
{
	need_resched = 0;
	boost_counter = 0;
}

void preempt_tick(void)
{
	struct task *cur = sched_current();
	if (!cur) return;

	/* Boost timer */
	boost_counter++;
	if (boost_counter >= MLFQ_BOOST_INTERVAL) {
		boost_counter = 0;
		mlfq_boost_all();
	}

	/* Decrement timeslice */
	if (cur->remaining > 0)
		cur->remaining--;

	if (cur->remaining <= 0) {
		/* Timeslice exhausted → flag for deferred reschedule in isr_dispatch */
		need_resched = 1;
	}
}

int preempt_need_reschedule(void)
{
	return need_resched;
}

void preempt_clear(void)
{
	need_resched = 0;
}
