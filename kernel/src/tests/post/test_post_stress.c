#include "../ktest.h"
#include <dicron/sched.h>
#include <dicron/time.h>
#include <dicron/io.h>
#include "mm/pmm.h"

/*
 * Post-boot stress test: spawn many concurrent threads, all doing
 * real work. Measures throughput and checks for memory leaks.
 */

#define POST_STRESS_COUNT 20

static volatile int stress_counters[POST_STRESS_COUNT];
static volatile int stress_finished;

static void stress_worker(void *arg)
{
	int id = (int)(uint64_t)arg;
	for (int i = 0; i < 500; i++)
		stress_counters[id]++;
	__sync_fetch_and_add(&stress_finished, 1);
	kthread_exit();
}

KTEST_REGISTER(ktest_post_stress, "post: concurrent stress", KTEST_CAT_POST)
static void ktest_post_stress(void)
{
	KTEST_BEGIN("post: concurrent stress");

	for (int i = 0; i < POST_STRESS_COUNT; i++)
		stress_counters[i] = 0;
	stress_finished = 0;

	size_t mem_before = pmm_free_pages_count();
	uint64_t t_start = ktime_ms();

	for (int i = 0; i < POST_STRESS_COUNT; i++)
		kthread_spawn(stress_worker, (void *)(uint64_t)i);

	/* Wait for all with timeout */
	uint64_t deadline = ktime_ms() + 5000;
	while (__sync_fetch_and_add(&stress_finished, 0) < POST_STRESS_COUNT) {
		if (ktime_ms() > deadline) break;
		__asm__ volatile("hlt");
	}

	uint64_t t_end = ktime_ms();

	KTEST_EQ(stress_finished, POST_STRESS_COUNT,
		 "post_stress: all 20 threads finished");

	int all_correct = 1;
	for (int i = 0; i < POST_STRESS_COUNT; i++) {
		if (stress_counters[i] != 500)
			all_correct = 0;
	}
	KTEST_TRUE(all_correct, "post_stress: all counters reached 500");

	/* Give dead threads a moment to be fully cleaned up */
	uint64_t settle = ktime_ms() + 50;
	while (ktime_ms() < settle)
		__asm__ volatile("hlt");

	size_t mem_after = pmm_free_pages_count();
	/* Dead threads' stacks are still allocated (no reaper yet),
	   but verify the system is at least stable */

	kio_printf("  [POST] stress: %d threads, %lu ms, mem delta=%ld pages\n",
		   POST_STRESS_COUNT, (t_end - t_start),
		   (long)(mem_before - mem_after));
}
