#include "ktest.h"
#include <dicron/sched.h>
#include "mm/kheap.h"
#include "lib/string.h"

static struct cpu_context *ctx_main;
static struct cpu_context *ctx_a;
static struct cpu_context *ctx_b;
static int counter_a;
static int counter_b;

static void thread_a(void)
{
	while (counter_a < 3) {
		counter_a++;
		switch_context(&ctx_a, ctx_b);
	}
	switch_context(&ctx_a, ctx_main);
}

static void thread_b(void)
{
	while (counter_b < 3) {
		counter_b++;
		switch_context(&ctx_b, ctx_a);
	}
}

KTEST_REGISTER(ktest_sched_context, "scheduler context", KTEST_CAT_BOOT)
static void ktest_sched_context(void)
{
	KTEST_BEGIN("context switch ping-pong");

	counter_a = 0;
	counter_b = 0;

	void *stack_a = kmalloc(4096);
	void *stack_b = kmalloc(4096);
	KTEST_NOT_NULL(stack_a, "stack A alloc");
	KTEST_NOT_NULL(stack_b, "stack B alloc");

	uint64_t *sp_a = (uint64_t *)((uint8_t *)stack_a + 4096
				      - sizeof(struct cpu_context));
	ctx_a = (struct cpu_context *)sp_a;
	memset(ctx_a, 0, sizeof(*ctx_a));
	ctx_a->rip = (uint64_t)thread_a;

	uint64_t *sp_b = (uint64_t *)((uint8_t *)stack_b + 4096
				      - sizeof(struct cpu_context));
	ctx_b = (struct cpu_context *)sp_b;
	memset(ctx_b, 0, sizeof(*ctx_b));
	ctx_b->rip = (uint64_t)thread_b;

	switch_context(&ctx_main, ctx_a);

	KTEST_EQ(counter_a, 3, "thread A ran 3 iterations");
	KTEST_EQ(counter_b, 3, "thread B ran 3 iterations");

	kfree(stack_a);
	kfree(stack_b);
}
