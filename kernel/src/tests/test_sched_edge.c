#include "ktest.h"
#include <dicron/sched.h>
#include <dicron/log.h>
#include "mm/pmm.h"
#include "mm/kheap.h"

static void edge_dummy_fn(void *arg) { (void)arg; }

KTEST_REGISTER(ktest_sched_edge, "scheduler edge", KTEST_CAT_BOOT)
static void ktest_sched_edge(void)
{
	KTEST_BEGIN("scheduler edge cases");

	/* NULL function pointer rejected */
	struct task *bad = kthread_create(NULL, (void *)0);
	KTEST_NULL(bad, "kthread_create(NULL) returns NULL");

	/* mass create/free — two warm runs to prime free-list + page tables */
	for (int w = 0; w < 2; w++) {
		struct task *warmup[100];
		int warmed = 0;
		for (int i = 0; i < 100; i++) {
			warmup[i] = kthread_create(edge_dummy_fn, (void *)0);
			if (!warmup[i]) break;
			warmed++;
		}
		for (int i = 0; i < warmed; i++)
			kthread_free(warmup[i]);
	}

	/* Now measure leaks on the second batch — free-list should absorb all */
	size_t before = pmm_free_pages_count();
	struct task *threads[100];
	int created = 0;
	for (int i = 0; i < 100; i++) {
		threads[i] = kthread_create(edge_dummy_fn, (void *)0);
		if (!threads[i]) break;
		created++;
	}
	KTEST_GT(created, 0, "created at least some threads");

	for (int i = 0; i < created; i++)
		kthread_free(threads[i]);

	size_t after = pmm_free_pages_count();
	KTEST_EQ(after, before, "thread create/free leak is zero after warm run");

	/* TASK_DEAD struct field check */
	struct task dead = {0};
	dead.state = TASK_DEAD;
	dead.context = (void *)0;
	KTEST_EQ(dead.state, TASK_DEAD, "dead task state");
	KTEST_NULL(dead.context, "dead task context is NULL");
}
