#include "pit.h"
#include <dicron/time.h>
#include <dicron/sched.h>
#include "arch/x86_64/idt.h"
#include "arch/x86_64/io.h"
#include "drivers/new/hpet/hpet.h"

static volatile uint64_t jiffies = 0;

static void pit_irq(struct idt_frame *frame)
{
	(void)frame;
	__sync_fetch_and_add(&jiffies, 1);
	preempt_tick();
}

void pit_init(void)
{
#ifdef CONFIG_HPET
	if (hpet_is_available())
		return;
#endif

	/*
	 * PIT base frequency is ~1193182 Hz.
	 * A divisor of 1193 yields 1193182 / 1193 = ~999.88 Hz (virtually 1kHz, 1ms).
	 */
	uint16_t divisor = 1193;

	/* Command port 0x43: channel 0, lobyte/hibyte, mode 3 (square wave), binary */
	outb(0x43, 0x36);
	outb(0x40, (uint8_t)(divisor & 0xFF));
	outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));

	idt_set_handler(0x20, pit_irq);
}

uint64_t ktime_ms(void)
{
#ifdef CONFIG_HPET
	if (hpet_is_available())
		return ktime_ns() / 1000000ULL;
#endif
	return jiffies;
}

void ksleep_ms(uint32_t ms)
{
#ifdef CONFIG_HPET
	if (hpet_is_available()) {
		ksleep_ns((uint64_t)ms * 1000000ULL);
		return;
	}
#endif

	uint64_t target = jiffies + ms;

	/*
	 * Sleep until target is reached. We issue 'hlt' to yield the CPU,
	 * waking up temporarily for every interrupt until the time arrives.
	 */
	while (jiffies < target)
		__asm__ volatile("hlt");
}
