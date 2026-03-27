#include "ktest.h"
#include <dicron/sched.h>
#include <dicron/time.h>
#include <dicron/io.h>

/*
 * Speed benchmark: measures time to create/enqueue/dequeue/free tasks.
 * Results are printed to console/serial for performance debugging.
 * Uses BOOT_TEST_SHOW=0 compatible printing — always prints speed stats.
 */

static void speed_dummy(void *arg) { (void)arg; }

#define SPEED_BATCH 100

KTEST_REGISTER(ktest_sched_speed, "scheduler speed benchmark", KTEST_CAT_BOOT)
static void ktest_sched_speed(void)
{
	KTEST_BEGIN("scheduler speed benchmark");

	/* --- benchmark 1: raw kthread_create + kthread_free --- */
	uint64_t t1 = ktime_ms();
	struct task *threads[SPEED_BATCH];
	for (int i = 0; i < SPEED_BATCH; i++)
		threads[i] = kthread_create(speed_dummy, NULL);
	for (int i = 0; i < SPEED_BATCH; i++)
		kthread_free(threads[i]);
	uint64_t t2 = ktime_ms();
	(void)t1; (void)t2;

	KTEST_PERF("%d kthread create+free: %lu ms", SPEED_BATCH, (t2 - t1));
	KTEST_TRUE(1, "speed: create/free benchmark ran");

	/* --- benchmark 2: MLFQ enqueue + pick_next cycle --- */
	/* Pre-create threads */
	for (int i = 0; i < SPEED_BATCH; i++)
		threads[i] = kthread_create(speed_dummy, NULL);

	uint64_t t3 = ktime_ms();
	for (int i = 0; i < SPEED_BATCH; i++)
		mlfq_enqueue(threads[i]);
	for (int i = 0; i < SPEED_BATCH; i++) {
		struct task *t = mlfq_pick_next();
		(void)t;
	}
	uint64_t t4 = ktime_ms();
	(void)t3; (void)t4;

	KTEST_PERF("%d MLFQ enqueue+pick: %lu ms", SPEED_BATCH, (t4 - t3));
	KTEST_TRUE(1, "speed: MLFQ cycle benchmark ran");

	for (int i = 0; i < SPEED_BATCH; i++)
		kthread_free(threads[i]);

	/* --- benchmark 3: MLFQ boost of scattered tasks --- */
	for (int i = 0; i < SPEED_BATCH; i++) {
		threads[i] = kthread_create(speed_dummy, NULL);
		threads[i]->priority = i % MLFQ_LEVELS;
		mlfq_enqueue(threads[i]);
	}

	uint64_t t5 = ktime_ms();
	mlfq_boost_all();
	uint64_t t6 = ktime_ms();
	(void)t5; (void)t6;

	KTEST_PERF("boost %d scattered tasks: %lu ms", SPEED_BATCH, (t6 - t5));
	KTEST_TRUE(1, "speed: boost benchmark ran");

	/* Drain and free */
	for (int i = 0; i < SPEED_BATCH; i++)
		mlfq_remove(threads[i]);
	for (int i = 0; i < SPEED_BATCH; i++)
		kthread_free(threads[i]);
}
