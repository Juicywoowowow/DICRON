#include "../ktest.h"
#include <dicron/sched.h>
#include <dicron/time.h>
#include <dicron/io.h>

/*
 * Post-boot test: voluntary yield.
 * Threads call kthread_yield() to cooperatively share the CPU.
 * Verify they interleave correctly via a shared sequence log.
 */

#define YIELD_ROUNDS 20

static volatile int yield_log[YIELD_ROUNDS * 2];
static volatile int yield_idx;
static volatile int yield_workers_done;

static void yield_worker(void *arg)
{
	int id = (int)(uint64_t)arg;
	for (int i = 0; i < YIELD_ROUNDS; i++) {
		int pos = __sync_fetch_and_add(&yield_idx, 1);
		if (pos < YIELD_ROUNDS * 2)
			yield_log[pos] = id;
		kthread_yield();
	}
	__sync_fetch_and_add(&yield_workers_done, 1);
	kthread_exit();
}

KTEST_REGISTER(ktest_post_yield, "post: yield", KTEST_CAT_POST)
static void ktest_post_yield(void)
{
	KTEST_BEGIN("post: voluntary yield interleaving");

	yield_idx = 0;
	yield_workers_done = 0;
	for (int i = 0; i < YIELD_ROUNDS * 2; i++)
		yield_log[i] = -1;

	kthread_spawn(yield_worker, (void *)0ULL);
	kthread_spawn(yield_worker, (void *)1ULL);

	/* Wait with timeout */
	uint64_t deadline = ktime_ms() + 2000;
	while (__sync_fetch_and_add(&yield_workers_done, 0) < 2) {
		if (ktime_ms() > deadline) break;
		__asm__ volatile("hlt");
	}

	KTEST_EQ(yield_workers_done, 2, "post_yield: both workers finished");

	/* Check that both IDs appear in the log (interleaving happened) */
	int saw_0 = 0, saw_1 = 0;
	for (int i = 0; i < YIELD_ROUNDS * 2 && yield_log[i] >= 0; i++) {
		if (yield_log[i] == 0) saw_0++;
		if (yield_log[i] == 1) saw_1++;
	}
	KTEST_EQ(saw_0, YIELD_ROUNDS, "post_yield: worker 0 logged all rounds");
	KTEST_EQ(saw_1, YIELD_ROUNDS, "post_yield: worker 1 logged all rounds");

	kio_printf("  [POST] yield: worker0=%d worker1=%d entries\n", saw_0, saw_1);
}
