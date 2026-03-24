#include "../ktest.h"
#include <dicron/sched.h>
#include <dicron/time.h>
#include <dicron/io.h>

/*
 * Post-boot test: kthread_exit removes task from run queue.
 * Spawn threads that exit immediately, verify the system stays stable
 * and the main thread continues running.
 */

static volatile int exit_count;

static void exit_worker(void *arg)
{
	(void)arg;
	__sync_fetch_and_add(&exit_count, 1);
	kthread_exit();
}

KTEST_REGISTER(ktest_post_exit, "post: exit", KTEST_CAT_POST)
static void ktest_post_exit(void)
{
	KTEST_BEGIN("post: kthread_exit cleanup");

	exit_count = 0;

	/* Spawn 10 threads that exit immediately */
	for (int i = 0; i < 10; i++)
		kthread_spawn(exit_worker, NULL);

	/* Wait for all to exit */
	uint64_t deadline = ktime_ms() + 2000;
	while (__sync_fetch_and_add(&exit_count, 0) < 10) {
		if (ktime_ms() > deadline) break;
		__asm__ volatile("hlt");
	}

	KTEST_EQ(exit_count, 10, "post_exit: all 10 threads exited");

	/* Verify system is still alive — spawn one more and run it */
	exit_count = 0;
	kthread_spawn(exit_worker, NULL);

	uint64_t d2 = ktime_ms() + 1000;
	while (__sync_fetch_and_add(&exit_count, 0) < 1) {
		if (ktime_ms() > d2) break;
		__asm__ volatile("hlt");
	}
	KTEST_EQ(exit_count, 1, "post_exit: system stable after mass exit");

	kio_printf("  [POST] exit: 11 threads exited cleanly\n");
}
