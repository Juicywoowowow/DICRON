#include "../ktest.h"
#include <dicron/sched.h>
#include <dicron/time.h>
#include <dicron/io.h>

/*
 * Post-boot test: verify timer preemption actually interrupts CPU-bound
 * threads and gives other threads a chance to run.
 *
 * Two threads both busy-loop incrementing counters. If preemption works,
 * both counters will advance. If not, only one will run and the other
 * stays at zero.
 */

static volatile int preempt_a;
static volatile int preempt_b;
static volatile int preempt_phase;

static void preempt_worker_a(void *arg)
{
	(void)arg;
	while (preempt_phase == 0) {
		preempt_a++;
	}
	kthread_exit();
}

static void preempt_worker_b(void *arg)
{
	(void)arg;
	while (preempt_phase == 0) {
		preempt_b++;
	}
	kthread_exit();
}

KTEST_REGISTER(ktest_post_preempt, "post: preemption", KTEST_CAT_POST)
static void ktest_post_preempt(void)
{
	KTEST_BEGIN("post: timer preemption fairness");

	preempt_a = 0;
	preempt_b = 0;
	preempt_phase = 0;

	kthread_spawn(preempt_worker_a, NULL);
	kthread_spawn(preempt_worker_b, NULL);

	/* Let them run for 100ms */
	uint64_t start = ktime_ms();
	while (ktime_ms() - start < 100)
		__asm__ volatile("hlt");

	/* Signal workers to stop */
	preempt_phase = 1;

	/* Give them a few timeslices to notice and exit */
	uint64_t wait_end = ktime_ms() + 200;
	while (ktime_ms() < wait_end)
		__asm__ volatile("hlt");

	/* Both must have run — if preemption failed, one would be 0 */
	KTEST_GT(preempt_a, 0, "post_preempt: worker A ran");
	KTEST_GT(preempt_b, 0, "post_preempt: worker B ran");

	kio_printf("  [POST] preempt: A=%d B=%d\n", preempt_a, preempt_b);
}
