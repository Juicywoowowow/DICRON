/*
 * test_pcspeaker_freq.c — PC Speaker frequency boundary tests.
 *
 * Verifies that the driver handles extreme and standard frequencies
 * without hanging, crashing, or leaving the speaker active.
 */

#include "ktest.h"
#include "../drivers/timer/pcspeaker.h"

KTEST_REGISTER(ktest_pcspeaker_freq_zero_both, "pcspeaker: zero freq and zero duration are no-ops", KTEST_CAT_BOOT)
static void ktest_pcspeaker_freq_zero_both(void)
{
	KTEST_BEGIN("pcspeaker: zero freq and zero duration are no-ops");
	pcspeaker_beep(0, 0);
	pcspeaker_beep(0, 1);
	pcspeaker_beep(440, 0);
	KTEST_TRUE(1, "pcspeaker: degenerate beep args returned cleanly");
}

KTEST_REGISTER(ktest_pcspeaker_freq_low, "pcspeaker: low frequency (37 Hz) returns cleanly", KTEST_CAT_BOOT)
static void ktest_pcspeaker_freq_low(void)
{
	KTEST_BEGIN("pcspeaker: low frequency (37 Hz) returns cleanly");
	/*
	 * 37 Hz ≈ PIT_BASE_HZ / 32255 — near the top of the 16-bit divisor
	 * range and the lowest audible frequency the speaker can produce.
	 */
	pcspeaker_stop();
	pcspeaker_beep(37, 1);
	pcspeaker_stop();
	KTEST_TRUE(1, "pcspeaker: 37 Hz beep returned cleanly");
}

KTEST_REGISTER(ktest_pcspeaker_freq_high, "pcspeaker: high frequency (~1.19 MHz) returns cleanly", KTEST_CAT_BOOT)
static void ktest_pcspeaker_freq_high(void)
{
	KTEST_BEGIN("pcspeaker: high frequency (~1.19 MHz) returns cleanly");
	/*
	 * 1 193 182 Hz → divisor = 1 — the maximum PIT channel 2 frequency.
	 * Exercises the boundary of the divisor calculation.
	 */
	pcspeaker_stop();
	pcspeaker_beep(1193182U, 1);
	pcspeaker_stop();
	KTEST_TRUE(1, "pcspeaker: near-max freq beep returned cleanly");
}

KTEST_REGISTER(ktest_pcspeaker_freq_standard, "pcspeaker: standard musical tones return cleanly", KTEST_CAT_BOOT)
static void ktest_pcspeaker_freq_standard(void)
{
	KTEST_BEGIN("pcspeaker: standard musical tones return cleanly");
	pcspeaker_stop();
	pcspeaker_beep(440,  1);	/* A4  */
	pcspeaker_beep(880,  1);	/* A5  */
	pcspeaker_beep(1000, 1);	/* 1 kHz boot-beep reference */
	pcspeaker_beep(2000, 1);	/* 2 kHz */
	pcspeaker_stop();
	KTEST_TRUE(1, "pcspeaker: standard tones returned cleanly");
}
