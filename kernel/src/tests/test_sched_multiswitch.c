#include "ktest.h"
#include <dicron/sched.h>
#include "mm/kheap.h"
#include "lib/string.h"

/*
 * Test rapidly switching between multiple kthread-created tasks.
 * Verifies that context isolation is maintained across many switches
 * and that per-thread arguments don't bleed across contexts.
 */

#define MULTI_COUNT 8

static struct cpu_context *multi_ctx_main;
static struct cpu_context *multi_ctx[MULTI_COUNT];
static volatile uint64_t multi_results[MULTI_COUNT];
static volatile int multi_next;

static void multi_fn(void *arg)
{
	uint64_t id = (uint64_t)arg;
	multi_results[id] = id * 0x1111;

	/* Yield back to main */
	switch_context(&multi_ctx[id], multi_ctx_main);

	/* Re-entered: update result to confirm second entry works */
	multi_results[id] += 1;
	switch_context(&multi_ctx[id], multi_ctx_main);
}

KTEST_REGISTER(ktest_sched_multiswitch, "scheduler multiswitch", KTEST_CAT_BOOT)
static void ktest_sched_multiswitch(void)
{
	KTEST_BEGIN("multi-thread interleaved switching");

	struct task *tasks[MULTI_COUNT];

	for (int i = 0; i < MULTI_COUNT; i++) {
		multi_results[i] = 0;
		tasks[i] = kthread_create(multi_fn, (void *)(uint64_t)i);
		KTEST_NOT_NULL(tasks[i], "multi: create thread");
		multi_ctx[i] = tasks[i]->context;
	}

	/* First pass: switch to each thread once */
	for (int i = 0; i < MULTI_COUNT; i++)
		switch_context(&multi_ctx_main, multi_ctx[i]);

	/* Verify each thread wrote its unique result */
	for (int i = 0; i < MULTI_COUNT; i++)
		KTEST_EQ(multi_results[i], (uint64_t)i * 0x1111,
			 "multi: first pass result correct");

	/* Second pass: re-enter each thread (they yield again) */
	for (int i = 0; i < MULTI_COUNT; i++)
		switch_context(&multi_ctx_main, multi_ctx[i]);

	/* Verify second-entry incremented the results */
	for (int i = 0; i < MULTI_COUNT; i++)
		KTEST_EQ(multi_results[i], (uint64_t)i * 0x1111 + 1,
			 "multi: second pass result correct");

	for (int i = 0; i < MULTI_COUNT; i++)
		kthread_free(tasks[i]);
}
