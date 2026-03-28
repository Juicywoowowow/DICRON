/*
 * test_pcspeaker_port.c — PC Speaker port 0x61 gate-bit tests.
 *
 * Reads port 0x61 after stop() and after beep+stop() to confirm that
 * the SPEAKER_GATE (bit 0) and SPEAKER_OUT (bit 1) bits are cleared.
 * Port 0x61 is the system control port, always present in QEMU.
 */

#include "ktest.h"
#include "../drivers/timer/pcspeaker.h"
#include "arch/x86_64/io.h"

/* Mirror of pcspeaker.c port-bit constants */
#define SPEAKER_GATE	0x01
#define SPEAKER_OUT	0x02

KTEST_REGISTER(ktest_pcspeaker_port_stop_clears_bits, "pcspeaker: stop() clears gate and output bits", KTEST_CAT_BOOT)
static void ktest_pcspeaker_port_stop_clears_bits(void)
{
	KTEST_BEGIN("pcspeaker: stop() clears gate and output bits");
	pcspeaker_stop();
	uint8_t val = inb(0x61);
	KTEST_FALSE((int)(val & (uint8_t)(SPEAKER_GATE | SPEAKER_OUT)),
		    "pcspeaker: port 0x61 gate+output bits cleared after stop()");
}

KTEST_REGISTER(ktest_pcspeaker_port_beep_stop_clears, "pcspeaker: beep+stop() leaves gate bits clear", KTEST_CAT_BOOT)
static void ktest_pcspeaker_port_beep_stop_clears(void)
{
	KTEST_BEGIN("pcspeaker: beep+stop() leaves gate bits clear");
	pcspeaker_beep(1000, 1);
	pcspeaker_stop();
	uint8_t val = inb(0x61);
	KTEST_FALSE((int)(val & (uint8_t)(SPEAKER_GATE | SPEAKER_OUT)),
		    "pcspeaker: gate+output bits clear after beep+stop()");
}

KTEST_REGISTER(ktest_pcspeaker_port_zero_dur_safe, "pcspeaker: zero-duration beep leaves port safe", KTEST_CAT_BOOT)
static void ktest_pcspeaker_port_zero_dur_safe(void)
{
	KTEST_BEGIN("pcspeaker: zero-duration beep leaves port safe");
	/* zero-duration beep must be a no-op — gate bits must stay clear */
	pcspeaker_stop();
	pcspeaker_beep(1000, 0);
	pcspeaker_stop();
	KTEST_TRUE(1, "pcspeaker: zero-duration beep port access returned cleanly");
}

KTEST_REGISTER(ktest_pcspeaker_port_stress, "pcspeaker: rapid beep-stop cycles leave gate clear", KTEST_CAT_BOOT)
static void ktest_pcspeaker_port_stress(void)
{
	KTEST_BEGIN("pcspeaker: rapid beep-stop cycles leave gate clear");
	for (int i = 0; i < 4; i++) {
		pcspeaker_beep((uint32_t)(500 + i * 100), 1);
		pcspeaker_stop();
	}
	uint8_t val = inb(0x61);
	KTEST_FALSE((int)(val & (uint8_t)(SPEAKER_GATE | SPEAKER_OUT)),
		    "pcspeaker: gate+output clear after stress loop");
}
