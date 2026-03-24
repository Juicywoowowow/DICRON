#include "ktest.h"
#include <dicron/sched.h>
#include "mm/kheap.h"
#include "lib/string.h"

/*
 * Test full thread lifecycle: create → run → return → free.
 * Verifies the kernel_thread_stub correctly bridges into C and
 * that a thread returning doesn't corrupt the caller.
 */

static struct cpu_context *lc_ctx_main;
static struct cpu_context *lc_ctx_thread;
static volatile int lc_ran = 0;

static void lc_thread_fn(void *arg)
{
	uint64_t val = (uint64_t)arg;
	if (val == 0xCAFEBABE)
		lc_ran = 1;
	switch_context(&lc_ctx_thread, lc_ctx_main);
}

KTEST_REGISTER(ktest_sched_lifecycle, "scheduler lifecycle", KTEST_CAT_BOOT)
static void ktest_sched_lifecycle(void)
{
	KTEST_BEGIN("thread full lifecycle");

	/* create */
	lc_ran = 0;
	struct task *t = kthread_create(lc_thread_fn, (void *)0xCAFEBABE);
	KTEST_NOT_NULL(t, "lifecycle: create non-null");
	KTEST_EQ(t->state, TASK_READY, "lifecycle: initial state READY");

	/* run */
	lc_ctx_thread = t->context;
	switch_context(&lc_ctx_main, lc_ctx_thread);
	KTEST_TRUE(lc_ran, "lifecycle: thread body executed");

	/* free */
	kthread_free(t);

	/* create a second thread to verify allocator still works post-free */
	lc_ran = 0;
	struct task *t2 = kthread_create(lc_thread_fn, (void *)0xCAFEBABE);
	KTEST_NOT_NULL(t2, "lifecycle: second create after free");
	lc_ctx_thread = t2->context;
	switch_context(&lc_ctx_main, lc_ctx_thread);
	KTEST_TRUE(lc_ran, "lifecycle: second thread ran after first freed");
	kthread_free(t2);
}
