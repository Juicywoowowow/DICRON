#ifndef _DICRON_ARCH_X86_64_TSS_H
#define _DICRON_ARCH_X86_64_TSS_H

#include <stdint.h>

/*
 * x86_64 Task State Segment.
 * In long mode, the TSS is primarily used to store the kernel stack
 * pointer (RSP0) for ring 3 → ring 0 transitions.
 */
struct tss {
	uint32_t reserved0;
	uint64_t rsp0;        /* kernel stack for ring 0 */
	uint64_t rsp1;
	uint64_t rsp2;
	uint64_t reserved1;
	uint64_t ist[7];      /* interrupt stack table */
	uint64_t reserved2;
	uint16_t reserved3;
	uint16_t iopb_offset;
} __attribute__((packed));

void tss_init(void);
void tss_set_rsp0(uint64_t rsp0);

#endif
