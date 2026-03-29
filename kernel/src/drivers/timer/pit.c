#include "pit.h"
#include <dicron/time.h>
#include <dicron/sched.h>
#include "arch/x86_64/idt.h"
#include "arch/x86_64/io.h"
#include "drivers/hpet/hpet.h"

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

uint64_t pit_jiffies(void)
{
	return jiffies;
}

void pit_sleep_ms(uint32_t ms)
{
	uint64_t target = jiffies + ms;

	/*
	 * Sleep until target is reached. We issue 'hlt' to yield the CPU,
	 * waking up temporarily for every interrupt until the time arrives.
	 */
	while (jiffies < target)
		__asm__ volatile("hlt");
}
