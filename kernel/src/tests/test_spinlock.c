#include "ktest.h"
#include <dicron/spinlock.h>

KTEST_REGISTER(ktest_spinlock, "spinlock basic ops", KTEST_CAT_BOOT)
static void ktest_spinlock(void)
{
	KTEST_BEGIN("spinlock basic ops");

	spinlock_t lock = SPINLOCK_INIT;

	KTEST_EQ(lock.locked, 0, "initial state unlocked");

	spin_lock(&lock);
	KTEST_EQ(lock.locked, 1, "locked after spin_lock");

	spin_unlock(&lock);
	KTEST_EQ(lock.locked, 0, "unlocked after spin_unlock");

	/* irqsave/restore round-trip */
	uint64_t flags = spin_lock_irqsave(&lock);
	KTEST_EQ(lock.locked, 1, "locked after irqsave");
	spin_unlock_irqrestore(&lock, flags);
	KTEST_EQ(lock.locked, 0, "unlocked after irqrestore");

	/* double lock/unlock sequence */
	spin_lock(&lock);
	spin_unlock(&lock);
	spin_lock(&lock);
	spin_unlock(&lock);
	KTEST_EQ(lock.locked, 0, "double lock/unlock cycle clean");
}
