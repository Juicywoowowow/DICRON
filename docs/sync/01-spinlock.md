Spinlocks
==========

1. Overview
-----------

Spinlocks provide simple mutual exclusion for synchronization in the
kernel. They are used to protect critical sections that must be held
for short durations.

2. Implementation
-----------------

Spinlocks are implemented using atomic operations on x86_64:
  - Use the 'lock' prefix for atomic operations
  - Test-and-set for acquiring lock
  - Simple loop for busy-waiting

3. API
-----

  void spin_init(spinlock_t *lock)
      - Initialize spinlock

  void spin_lock(spinlock_t *lock)
      - Acquire lock (busy-wait if held)

  void spin_unlock(spinlock_t *lock)
      - Release lock

  int spin_trylock(spinlock_t *lock)
      - Try to acquire lock (non-blocking)
      - Returns 1 if acquired, 0 if held

4. Usage Guidelines
------------------

Spinlocks should be used when:
  - Critical section is very short
  - No sleeping/blocking is needed
  - Lock is held for brief duration

Spinlocks should NOT be used when:
  - Interrupt handlers need synchronization (use IRQ-safe variants)
  - Long operations are performed while holding lock
  - Code that may sleep is needed (use mutex instead)

5. IRQ-Safe Spinlocks
---------------------

For code that runs in interrupt context:
  - Disable interrupts before acquiring
  - Re-enable after releasing

6. Preemption and Spinlocks
---------------------------

When a thread holds a spinlock:
  - Preemption is typically disabled
  - Prevents priority inversion issues
