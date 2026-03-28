/*
 * pcspeaker.c — PC Speaker driver (PIT channel 2).
 *
 * Programs PIT channel 2 as a square-wave oscillator and gates it through
 * the system control port (0x61) to drive the PC speaker.
 *
 * Timing uses HPET when CONFIG_HPET is enabled (free-running counter,
 * safe with interrupts disabled).  Falls back to a ktime_ms() busy-wait
 * in normal context, or a calibrated busy-loop when called from panic.
 */

#include "pcspeaker.h"
#include "arch/x86_64/io.h"
#include <dicron/log.h>
#include <dicron/time.h>
#include <generated/autoconf.h>

#ifdef CONFIG_PCSPEAKER

#ifdef CONFIG_HPET
#include "drivers/new/hpet/hpet.h"
#endif

/* PIT oscillator base frequency (Hz) */
#define PIT_BASE_HZ 1193182UL

/* Port 0x61 bit masks */
#define SPEAKER_GATE 0x01 /* PIT channel 2 gate enable */
#define SPEAKER_OUT 0x02  /* speaker output enable     */

/* ── internal helpers ── */

static void pcspeaker_set_freq(uint32_t freq_hz) {
  uint32_t divisor;

  if (freq_hz == 0)
    return;

  divisor = (uint32_t)(PIT_BASE_HZ / (unsigned long)freq_hz);

  /* Channel 2, lobyte/hibyte, mode 3 (square wave), binary */
  outb(0x43, 0xB6);
  outb(0x42, (uint8_t)(divisor & 0xFF));
  outb(0x42, (uint8_t)((divisor >> 8) & 0xFF));
}

/*
 * Delay for ms milliseconds.  Works with interrupts disabled when the HPET
 * is available (ksleep_ns busy-spins on the HPET counter).  Falls back to
 * a raw busy-loop that is deliberately conservative — it will over-wait
 * rather than under-wait on slower machines.
 */
static void pcspeaker_delay_ms(uint32_t ms) {
#ifdef CONFIG_HPET
  if (hpet_is_available()) {
    ksleep_ns((uint64_t)ms * 1000000ULL);
    return;
  }
#endif
  /*
   * Fallback busy-loop.  ~200 000 iterations ≈ 1 ms on a 2 GHz core
   * executing ~4 cycles per iteration (mov + cmp + branch + pause).
   * Errs on the side of longer waits; fine for beep tones.
   */
  for (volatile uint64_t i = 0; i < (uint64_t)ms * 200000ULL; i++)
    ;
}

/* ── public API ── */

void pcspeaker_stop(void) {
  uint8_t val = inb(0x61);
  val = (uint8_t)(val & (uint8_t)~(SPEAKER_GATE | SPEAKER_OUT));
  outb(0x61, val);
}

void pcspeaker_beep(uint32_t freq_hz, uint32_t duration_ms) {
  uint8_t val;

  if (freq_hz == 0 || duration_ms == 0)
    return;

  pcspeaker_set_freq(freq_hz);

  val = inb(0x61);
  val |= (uint8_t)(SPEAKER_GATE | SPEAKER_OUT);
  outb(0x61, val);

  pcspeaker_delay_ms(duration_ms);

  pcspeaker_stop();
}

/*
 * Three descending tones — distinguishable from a normal boot beep.
 * Fully interrupt-safe: ksleep_ns() uses 'pause', not 'hlt'.
 */
void pcspeaker_panic_beep(void) {
  pcspeaker_beep(1200, 120);
  pcspeaker_delay_ms(60);
  pcspeaker_beep(900, 120);
  pcspeaker_delay_ms(60);
  pcspeaker_beep(600, 250);
}

/*
 * Boot self-test: one short beep at 1 kHz.
 * Called once after PIT/HPET are both initialised.
 */
void pcspeaker_init(void) {
  pcspeaker_stop();
  pcspeaker_beep(1000, 60);
  klog(KLOG_INFO, "pcspeaker: initialized\n");
}

#endif /* CONFIG_PCSPEAKER */
