#include "../ktest.h"
#include <dicron/sched.h>
#include <dicron/time.h>
#include <dicron/io.h>

/*
 * Post-boot test: spawn threads and verify they actually run
 * via timer-driven preemption.
 */

static volatile int spawn_counters[4];
static volatile int spawn_done;

static void spawn_worker(void *arg)
{
	int id = (int)(uint64_t)arg;
	for (int i = 0; i < 1000; i++)
		spawn_counters[id]++;
	__sync_fetch_and_add(&spawn_done, 1);
	kthread_exit();
}

KTEST_REGISTER(ktest_post_spawn, "post: spawn + preemptive run", KTEST_CAT_POST)
static void ktest_post_spawn(void)
{
	KTEST_BEGIN("post: spawn + preemptive run");

	for (int i = 0; i < 4; i++)
		spawn_counters[i] = 0;
	spawn_done = 0;

	for (int i = 0; i < 4; i++) {
		struct task *t = kthread_spawn(spawn_worker, (void *)(uint64_t)i);
		KTEST_NOT_NULL(t, "post_spawn: thread created");
	}

	/* Wait for all 4 workers with timeout */
	uint64_t deadline = ktime_ms() + 2000;
	while (__sync_fetch_and_add(&spawn_done, 0) < 4) {
		if (ktime_ms() > deadline) break;
		__asm__ volatile("hlt");
	}

	KTEST_EQ(spawn_done, 4, "post_spawn: all 4 workers finished");
	for (int i = 0; i < 4; i++)
		KTEST_EQ(spawn_counters[i], 1000, "post_spawn: worker counter reached 1000");

	kio_printf("  [POST] spawn: %d/%d/%d/%d\n",
		   spawn_counters[0], spawn_counters[1],
		   spawn_counters[2], spawn_counters[3]);
}
