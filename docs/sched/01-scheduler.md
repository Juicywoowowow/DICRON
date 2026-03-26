Scheduler
=========

1. Overview
-----------

The scheduler is responsible for time-sharing CPU resources among
multiple tasks. DICRON implements a Multilevel Feedback Queue (MLFQ)
scheduler with preemptive multitasking.

2. Task States
--------------

Tasks can be in one of the following states:

  TASK_RUNNING - Currently executing on CPU
  TASK_READY   - Waiting in run queue
  TASK_SLEEPING - Blocked on wait queue
  TASK_DEAD    - Finished, waiting for cleanup

3. MLFQ Design
--------------

The scheduler uses 4 priority levels (queues):

  Level 0: Highest priority, 2 ms timeslice
  Level 1:             4 ms timeslice
  Level 2:             8 ms timeslice
  Level 3: Lowest priority, 16 ms timeslice

New tasks start at highest priority (level 0). After using their
timeslice, they are demoted to the next lower level. This provides
good response time for interactive tasks while fairly distributing
CPU among CPU-bound tasks.

4. Priority Boosting
--------------------

To prevent starvation of low-priority tasks, the scheduler boosts
all tasks to highest priority every 500 ms (configurable via
MLFQ_BOOST_INTERVAL).

5. Kernel Threads
-----------------

Kernel threads (kthread) are lightweight threads that run in kernel
mode. They have no associated userspace process (process field is NULL).
Kernel threads are used for deferred work and background tasks.

6. API
-----

Kernel thread management:
  struct task *kthread_create(void (*fn)(void *), void *arg)
      - Create a new kernel thread that starts paused
      - Returns task pointer

  struct task *kthread_spawn(void (*fn)(void *), void *arg)
      - Create and start a new kernel thread

  void kthread_exit(void)
      - Terminate current kernel thread

  void kthread_yield(void)
      - Voluntarily give up CPU to other tasks

Kernel stack management:
  void *kstack_alloc(size_t pages)
      - Allocate kernel stack of given size

  void kstack_free(void *stack, size_t pages)
      - Free kernel stack

  void kthread_free(struct task *t)
      - Free a terminated task and its stack

Scheduler control:
  void sched_init(void)
      - Initialize scheduler

  void schedule(void)
      - Run the scheduler to pick next task

  struct task *sched_current(void)
      - Get currently running task

MLFQ operations:
  void mlfq_init(void)
      - Initialize MLFQ queues

  void mlfq_enqueue(struct task *t)
      - Add task to its appropriate queue

  void mlfq_remove(struct task *t)
      - Remove task from its queue

  void mlfq_demote(struct task *t)
      - Move task to lower priority queue

  void mlfq_boost_all(void)
      - Boost all tasks to highest priority

  struct task *mlfq_pick_next(void)
      - Get next task to run

  int mlfq_queue_length(int level)
      - Get number of tasks in a queue

Preemption:
  void preempt_init(void)
      - Initialize preemptive scheduling

  void preempt_tick(void)
      - Called on each timer tick

  int preempt_need_reschedule(void)
      - Check if reschedule is needed

  void preempt_clear(void)
      - Clear reschedule flag

7. Context Switching
--------------------

The actual CPU context switch is performed by switch_context() in
switch.asm. It saves and restores all callee-saved registers:

  r15, r14, r13, r12, rbx, rbp, rip

8. Wait Queues
--------------

Tasks can block on wait queues for synchronization:

  void kwait(struct wait_queue *wq)
      - Block current task on wait queue

  void kwake(struct wait_queue *wq)
      - Wake one task from wait queue

  int kwake_all(struct wait_queue *wq)
      - Wake all tasks from wait queue

9. Preemptive Scheduling
------------------------

The scheduler uses the PIT (Programmable Interval Timer) to generate
timer interrupts at 1000 Hz (1 ms intervals). On each tick,
preempt_tick() is called to manage timeslice accounting.
