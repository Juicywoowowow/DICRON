#ifndef _DICRON_ARCH_X86_64_IDT_H
#define _DICRON_ARCH_X86_64_IDT_H

#include <stdint.h>

struct idt_frame {
	uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
	uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
	uint64_t int_no, err_code;
	uint64_t rip, cs, rflags, rsp, ss;
};

typedef void (*irq_handler_t)(struct idt_frame *frame);

void idt_init(void);
void idt_set_handler(uint8_t vec, irq_handler_t handler);

#endif
