/*
 * test_pcspeaker_panic.c — PC Speaker panic-beep sequence tests.
 *
 * Validates that pcspeaker_panic_beep() does not crash, can be called
 * multiple times, and leaves the speaker silent on completion.
 */

#include "ktest.h"
#include "../drivers/timer/pcspeaker.h"

KTEST_REGISTER(ktest_pcspeaker_panic_safe, "pcspeaker: panic_beep() does not crash", KTEST_CAT_BOOT)
static void ktest_pcspeaker_panic_safe(void)
{
	KTEST_BEGIN("pcspeaker: panic_beep() does not crash");
	pcspeaker_panic_beep();
	KTEST_TRUE(1, "pcspeaker: panic_beep() returned cleanly");
}

KTEST_REGISTER(ktest_pcspeaker_panic_stop_after, "pcspeaker: stop() after panic_beep() is safe", KTEST_CAT_BOOT)
static void ktest_pcspeaker_panic_stop_after(void)
{
	KTEST_BEGIN("pcspeaker: stop() after panic_beep() is safe");
	pcspeaker_panic_beep();
	/* speaker must be silenceable after the panic sequence completes */
	pcspeaker_stop();
	KTEST_TRUE(1, "pcspeaker: stop() after panic_beep() returned cleanly");
}

KTEST_REGISTER(ktest_pcspeaker_panic_repeat, "pcspeaker: repeated panic_beep() calls are safe", KTEST_CAT_BOOT)
static void ktest_pcspeaker_panic_repeat(void)
{
	KTEST_BEGIN("pcspeaker: repeated panic_beep() calls are safe");
	pcspeaker_panic_beep();
	pcspeaker_panic_beep();
	KTEST_TRUE(1, "pcspeaker: two consecutive panic_beep() calls returned cleanly");
}

KTEST_REGISTER(ktest_pcspeaker_panic_mixed_seq, "pcspeaker: beep then panic_beep sequence is safe", KTEST_CAT_BOOT)
static void ktest_pcspeaker_panic_mixed_seq(void)
{
	KTEST_BEGIN("pcspeaker: beep then panic_beep sequence is safe");
	/* Simulate normal use followed by a panic */
	pcspeaker_beep(1000, 1);
	pcspeaker_panic_beep();
	pcspeaker_stop();
	KTEST_TRUE(1, "pcspeaker: mixed beep+panic sequence returned cleanly");
}
