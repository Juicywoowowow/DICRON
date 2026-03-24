#include "ktest.h"
#include <dicron/sched.h>
#include "mm/kheap.h"
#include "lib/string.h"

static struct cpu_context *kt_ctx_main;
static struct cpu_context *kt_ctx_thread;
static int kt_arg_received;

static void kt_test_fn(void *arg)
{
	uint64_t val = (uint64_t)arg;
	if (val == 0xDEADBEEF)
		kt_arg_received = 1;
	switch_context(&kt_ctx_thread, kt_ctx_main);
}

KTEST_REGISTER(ktest_sched_kthread, "scheduler kthread", KTEST_CAT_BOOT)
static void ktest_sched_kthread(void)
{
	KTEST_BEGIN("kthread_create + argument passing");

	kt_arg_received = 0;

	struct task *t = kthread_create(kt_test_fn, (void *)0xDEADBEEF);
	KTEST_NOT_NULL(t, "kthread_create non-null");
	KTEST_EQ(t->state, TASK_READY, "state is TASK_READY");
	KTEST_GE(t->pid, 0, "PID >= 0");

	kt_ctx_thread = t->context;
	switch_context(&kt_ctx_main, kt_ctx_thread);

	KTEST_TRUE(kt_arg_received, "thread received 0xDEADBEEF arg");

	kthread_free(t);
}
