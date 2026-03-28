#ifndef _DICRON_DRIVERS_TIMER_PCSPEAKER_H
#define _DICRON_DRIVERS_TIMER_PCSPEAKER_H

#include <stdint.h>
#include <generated/autoconf.h>

#ifdef CONFIG_PCSPEAKER

/*
 * PC Speaker driver — PIT channel 2 tone generation.
 *
 * Timing strategy:
 *   CONFIG_HPET enabled → ksleep_ns() (spins on HPET counter, works with
 *                          interrupts disabled, used from kpanic).
 *   HPET absent         → ktime_ms() busy-wait (normal context only) or
 *                          calibrated busy-loop (panic context).
 *
 * Port map:
 *   0x43 — PIT command register (write)
 *   0x42 — PIT channel 2 data  (write)
 *   0x61 — System control port; bit 0 = gate, bit 1 = speaker output
 */

/* Initialize the speaker and emit a short boot self-test beep. */
void pcspeaker_init(void);

/* Sound the speaker at freq_hz for duration_ms milliseconds. */
void pcspeaker_beep(uint32_t freq_hz, uint32_t duration_ms);

/* Immediately silence the speaker. */
void pcspeaker_stop(void);

/*
 * Emit the panic beep sequence (three descending tones).
 * Safe to call from kpanic() with interrupts disabled.
 */
void pcspeaker_panic_beep(void);

#else  /* !CONFIG_PCSPEAKER */

static inline void pcspeaker_init(void) {}
static inline void pcspeaker_beep(uint32_t f, uint32_t d) { (void)f; (void)d; }
static inline void pcspeaker_stop(void) {}
static inline void pcspeaker_panic_beep(void) {}

#endif /* CONFIG_PCSPEAKER */

#endif /* _DICRON_DRIVERS_TIMER_PCSPEAKER_H */
