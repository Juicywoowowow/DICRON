#ifndef _DICRON_SPINLOCK_H
#define _DICRON_SPINLOCK_H

#include <stdint.h>

/*
 * Simple test-and-set spinlock with interrupt save/restore.
 *
 * spin_lock / spin_unlock:
 *   Use when interrupts are already disabled (e.g. inside an ISR).
 *
 * spin_lock_irqsave / spin_unlock_irqrestore:
 *   Use from normal kernel context — saves RFLAGS, disables interrupts,
 *   acquires the lock. Restore undoes everything.
 */

typedef struct {
	volatile int locked;
} spinlock_t;

#define SPINLOCK_INIT { .locked = 0 }

static inline void spin_lock(spinlock_t *lock)
{
	for (;;) {
		if (!__sync_lock_test_and_set(&lock->locked, 1))
			break;
		/* TTAS: wait until the cache line signals the lock might be free */
		while (lock->locked)
			__asm__ volatile("pause" ::: "memory");
	}
}

static inline void spin_unlock(spinlock_t *lock)
{
	__sync_lock_release(&lock->locked);
}

static inline uint64_t spin_lock_irqsave(spinlock_t *lock)
{
	uint64_t flags;

	__asm__ volatile(
		"pushfq\n\t"
		"pop %0\n\t"
		"cli"
		: "=r"(flags)
		:
		: "memory"
	);
	spin_lock(lock);
	return flags;
}

static inline void spin_unlock_irqrestore(spinlock_t *lock, uint64_t flags)
{
	spin_unlock(lock);
	__asm__ volatile(
		"push %0\n\t"
		"popfq"
		:
		: "r"(flags)
		: "memory", "cc"
	);
}

#endif
