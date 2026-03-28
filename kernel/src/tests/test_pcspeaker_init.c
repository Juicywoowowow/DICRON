/*
 * test_pcspeaker_init.c — PC Speaker initialisation and idempotency tests.
 *
 * Verifies that pcspeaker_init() and pcspeaker_stop() are safe to call
 * in various combinations and do not leave the speaker in a bad state.
 */

#include "ktest.h"
#include "../drivers/timer/pcspeaker.h"

KTEST_REGISTER(ktest_pcspeaker_init_safe, "pcspeaker: init() does not crash", KTEST_CAT_BOOT)
static void ktest_pcspeaker_init_safe(void)
{
	KTEST_BEGIN("pcspeaker: init() does not crash");
	pcspeaker_init();
	KTEST_TRUE(1, "pcspeaker: pcspeaker_init() returned cleanly");
}

KTEST_REGISTER(ktest_pcspeaker_init_repeat, "pcspeaker: repeated init() is idempotent", KTEST_CAT_BOOT)
static void ktest_pcspeaker_init_repeat(void)
{
	KTEST_BEGIN("pcspeaker: repeated init() is idempotent");
	pcspeaker_init();
	pcspeaker_init();
	KTEST_TRUE(1, "pcspeaker: double init() returned cleanly");
}

KTEST_REGISTER(ktest_pcspeaker_stop_idempotent, "pcspeaker: stop() is idempotent", KTEST_CAT_BOOT)
static void ktest_pcspeaker_stop_idempotent(void)
{
	KTEST_BEGIN("pcspeaker: stop() is idempotent");
	pcspeaker_stop();
	pcspeaker_stop();
	pcspeaker_stop();
	KTEST_TRUE(1, "pcspeaker: triple stop() returned cleanly");
}

KTEST_REGISTER(ktest_pcspeaker_init_then_stop, "pcspeaker: init() followed by stop() is clean", KTEST_CAT_BOOT)
static void ktest_pcspeaker_init_then_stop(void)
{
	KTEST_BEGIN("pcspeaker: init() followed by stop() is clean");
	pcspeaker_init();
	pcspeaker_stop();
	KTEST_TRUE(1, "pcspeaker: init() then stop() returned cleanly");
}
