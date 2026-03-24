#ifndef _DICRON_SCHED_H
#define _DICRON_SCHED_H

#include <dicron/types.h>

/* --- task states --- */
enum task_state {
	TASK_RUNNING = 0,
	TASK_READY,
	TASK_SLEEPING,
	TASK_DEAD
};

/* --- MLFQ configuration --- */
#define MLFQ_LEVELS          4
#define MLFQ_BOOST_INTERVAL  500   /* ms between full priority boosts */

/* Per-level timeslice in ticks (1 tick = 1ms with PIT at 1kHz) */
static const int mlfq_timeslice[MLFQ_LEVELS] = { 2, 4, 8, 16 };

/*
 * x86_64 callee-saved registers context.
 * Must match the push/pop order in switch_context (switch.asm).
 */
struct cpu_context {
	uint64_t r15;
	uint64_t r14;
	uint64_t r13;
	uint64_t r12;
	uint64_t rbx;
	uint64_t rbp;
	uint64_t rip;
} __attribute__((packed));

/* --- task struct --- */
struct process; /* forward declaration */

struct task {
	pid_t pid;
	enum task_state state;
	void *kernel_stack;
	struct cpu_context *context;
	uint64_t page_table;

	/* owning process (NULL for pure kernel threads) */
	struct process *process;

	/* MLFQ scheduling */
	int priority;      /* current queue level (0 = highest) */
	int remaining;     /* ticks left in current timeslice */

	/* doubly-linked list for MLFQ queues */
	struct task *next;
	struct task *prev;
};

/* --- wait queue --- */
struct wait_queue {
	struct task *head;
	struct task *tail;
};

#define WAIT_QUEUE_INIT { .head = NULL, .tail = NULL }

/* --- arch/x86_64/switch.asm --- */
extern void switch_context(struct cpu_context **old, struct cpu_context *new_ctx);

/* --- sched/task.c --- */
struct task *kthread_create(void (*fn)(void *), void *arg);
struct task *kthread_spawn(void (*fn)(void *), void *arg);
void kthread_exit(void);
void kthread_yield(void);
void *kstack_alloc(size_t pages);
void kstack_free(void *stack, size_t pages);
void kthread_free(struct task *t);

/* --- sched/sched.c --- */
void sched_init(void);
void schedule(void);
struct task *sched_current(void);

/* --- sched/mlfq.c --- */
void mlfq_init(void);
void mlfq_enqueue(struct task *t);
void mlfq_remove(struct task *t);
void mlfq_demote(struct task *t);
void mlfq_boost_all(void);
struct task *mlfq_pick_next(void);
int mlfq_queue_length(int level);

/* --- sched/preempt.c --- */
void preempt_init(void);
void preempt_tick(void);
int preempt_need_reschedule(void);
void preempt_clear(void);

/* --- sched/waitqueue.c --- */
void kwait(struct wait_queue *wq);
void kwake(struct wait_queue *wq);
int kwake_all(struct wait_queue *wq);

#endif
