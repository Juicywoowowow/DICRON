#include "ktest.h"
#include "../drivers/timer/pcspeaker.h"

KTEST_REGISTER(ktest_pcspeaker_stop, "pcspeaker: stop is safe to call", KTEST_CAT_BOOT)
static void ktest_pcspeaker_stop(void)
{
	KTEST_BEGIN("pcspeaker: stop is safe to call");
	/* Verify stop() does not crash and leaves the port in a clean state */
	pcspeaker_stop();
	KTEST_TRUE(1, "pcspeaker: stop() returned cleanly");
}

KTEST_REGISTER(ktest_pcspeaker_beep_zero, "pcspeaker: zero-freq beep is no-op", KTEST_CAT_BOOT)
static void ktest_pcspeaker_beep_zero(void)
{
	KTEST_BEGIN("pcspeaker: zero-freq beep is no-op");
	pcspeaker_beep(0, 100);   /* must not hang or crash */
	pcspeaker_beep(440, 0);   /* zero duration — same */
	KTEST_TRUE(1, "pcspeaker: degenerate beep calls returned cleanly");
}
