#include "ktest.h"
#include "arch/x86_64/idt.h"

static volatile int soft_irq_caught = 0;

static void test_irq_handler(struct idt_frame *frame)
{
	(void)frame;
	soft_irq_caught = 1;
}

KTEST_REGISTER(ktest_idt_soft_irq, "IDT soft IRQ", KTEST_CAT_BOOT)
static void ktest_idt_soft_irq(void)
{
	KTEST_BEGIN("IDT software interrupt routing");

	idt_set_handler(0x2F, test_irq_handler);

	soft_irq_caught = 0;
	__asm__ volatile("int $0x2F");
	KTEST_TRUE(soft_irq_caught, "int 0x2F routed to handler");

	/* unregister and verify cleanup */
	idt_set_handler(0x2F, (irq_handler_t)0);

	/* double-register a handler */
	idt_set_handler(0x2E, test_irq_handler);
	soft_irq_caught = 0;
	__asm__ volatile("int $0x2E");
	KTEST_TRUE(soft_irq_caught, "int 0x2E routed correctly");
	idt_set_handler(0x2E, (irq_handler_t)0);
}
