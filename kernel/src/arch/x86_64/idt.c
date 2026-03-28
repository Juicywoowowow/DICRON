#include "idt.h"
#include "io.h"
#include "mm/swap.h"
#include "mm/vmm.h"
#include <dicron/sched.h>
#include <dicron/io.h>
#include <dicron/log.h>
#include <dicron/panic.h>
#include <generated/autoconf.h>
#include <stddef.h>

#define IDT_ENTRIES 256

/* PIC ports */
#define PIC1_CMD  0x20
#define PIC1_DATA 0x21
#define PIC2_CMD  0xA0
#define PIC2_DATA 0xA1

struct idt_entry {
	uint16_t offset_low;
	uint16_t selector;
	uint8_t  ist;
	uint8_t  type_attr;
	uint16_t offset_mid;
	uint32_t offset_high;
	uint32_t zero;
} __attribute__((packed));

struct idt_ptr {
	uint16_t limit;
	uint64_t base;
} __attribute__((packed));

static struct idt_entry idt[IDT_ENTRIES];
static struct idt_ptr   idtr;
static irq_handler_t    handlers[IDT_ENTRIES];

/* asm stubs — defined in idt_stubs.asm */
extern void *isr_stub_table[];

static void idt_set(uint8_t vec, uint64_t handler, uint8_t type_attr)
{
	idt[vec].offset_low  = (uint16_t)(handler & 0xFFFF);
	idt[vec].selector    = 0x08; /* kernel code segment */
	idt[vec].ist         = 0;
	idt[vec].type_attr   = type_attr;
	idt[vec].offset_mid  = (uint16_t)((handler >> 16) & 0xFFFF);
	idt[vec].offset_high = (uint32_t)((handler >> 32) & 0xFFFFFFFF);
	idt[vec].zero        = 0;
}

static void pic_remap(void)
{
	outb(PIC1_CMD, 0x11);
	io_wait();
	outb(PIC2_CMD, 0x11);
	io_wait();
	outb(PIC1_DATA, 0x20); /* IRQ 0-7  → int 0x20-0x27 */
	io_wait();
	outb(PIC2_DATA, 0x28); /* IRQ 8-15 → int 0x28-0x2F */
	io_wait();
	outb(PIC1_DATA, 0x04);
	io_wait();
	outb(PIC2_DATA, 0x02);
	io_wait();
	outb(PIC1_DATA, 0x01);
	io_wait();
	outb(PIC2_DATA, 0x01);
	io_wait();

	/* mask all except IRQ0 (PIT timer) and IRQ1 (keyboard) */
	outb(PIC1_DATA, 0xFC);
	outb(PIC2_DATA, 0xFF);
}

void idt_set_handler(uint8_t vec, irq_handler_t handler)
{
	handlers[vec] = handler;
}

/* called from asm stub */
void isr_dispatch(struct idt_frame *frame)
{
	if (handlers[frame->int_no])
		handlers[frame->int_no](frame);

	/* send EOI to PIC */
	if (frame->int_no >= 0x20 && frame->int_no < 0x30) {
		if (frame->int_no >= 0x28)
			outb(PIC2_CMD, 0x20);
		outb(PIC1_CMD, 0x20);
	}

	/* Deferred preemption (Option B): check after EOI */
	if (preempt_need_reschedule()) {
		preempt_clear();
		schedule();
	}
}

static void page_fault_handler(struct idt_frame *frame)
{
	uint64_t cr2;
	__asm__ volatile("mov %%cr2, %0" : "=r"(cr2));

	int from_user = (frame->cs & 3) == 3;

#ifdef CONFIG_SWAP
	/*
	 * Before treating the fault as fatal, check whether the faulting
	 * address was swap-evicted.  swap_pf_handle() reads the raw PTE:
	 * if it carries a swap marker it allocates a frame, reads the page
	 * back from ZRAM or disk, writes a new present PTE, and returns 1.
	 * Execution then retries the faulting instruction transparently.
	 */
	if (swap_pf_handle(cr2, vmm_get_cr3()))
		return;
#endif

	kio_printf("\n[!!!] PAGE FAULT%s\n", from_user ? " (userspace)" : " (kernel)");
	kio_printf("  Address: 0x%016lx\n", cr2);
	kio_printf("  RIP:     0x%016lx\n", frame->rip);
	kio_printf("  Error:   0x%lx [", frame->err_code);
	if (frame->err_code & 1) kio_printf("present ");
	if (frame->err_code & 2) kio_printf("write ");
	if (frame->err_code & 4) kio_printf("user ");
	if (frame->err_code & 8) kio_printf("reserved ");
	if (frame->err_code & 16) kio_printf("ifetch ");
	kio_printf("]\n");

	if (from_user) {
		klog(KLOG_ERR, "Killing user thread (page fault at 0x%lx)\n", cr2);
		kthread_exit();
	}

	kpanic("unhandled kernel page fault");
}

static void gpf_handler(struct idt_frame *frame)
{
	int from_user = (frame->cs & 3) == 3;

	kio_printf("\n[!!!] GENERAL PROTECTION FAULT%s\n",
		   from_user ? " (userspace)" : " (kernel)");
	kio_printf("  RIP:   0x%016lx\n", frame->rip);
	kio_printf("  Error: 0x%lx\n", frame->err_code);
	kio_printf("  CS:    0x%lx  SS: 0x%lx\n", frame->cs, frame->ss);

	if (from_user) {
		klog(KLOG_ERR, "Killing user thread (GPF at RIP 0x%lx)\n", frame->rip);
		kthread_exit();
	}

	kpanic("unhandled kernel GPF");
}

static void unhandled_fault_handler(struct idt_frame *frame)
{
	int from_user = (frame->cs & 3) == 3;
	kio_printf("\n[!!!] UNHANDLED CPU EXCEPTION %d%s\n",
		   (int)frame->int_no, from_user ? " (userspace)" : " (kernel)");
	kio_printf("  RIP:   0x%016lx\n", frame->rip);
	kio_printf("  Error: 0x%lx\n", frame->err_code);
	kio_printf("  CS:    0x%lx  SS: 0x%lx\n", frame->cs, frame->ss);

	if (from_user) {
		klog(KLOG_ERR, "Killing user thread (Exception %d)\n", (int)frame->int_no);
		kthread_exit();
	}
	kpanic("unhandled kernel exception");
}

void idt_init(void)
{
	for (int i = 0; i < IDT_ENTRIES; i++) {
		handlers[i] = NULL;
		if (i < 48)
			idt_set((uint8_t)i, (uint64_t)isr_stub_table[i], 0x8E);
	}

	/* Register CPU exception handlers */
	for (int i = 0; i < 32; i++) {
		handlers[i] = unhandled_fault_handler;
	}
	handlers[13] = gpf_handler;
	handlers[14] = page_fault_handler;

	pic_remap();

	idtr.limit = sizeof(idt) - 1;
	idtr.base  = (uint64_t)&idt;

	__asm__ volatile("lidt %0" : : "m"(idtr));
	__asm__ volatile("sti");
}
