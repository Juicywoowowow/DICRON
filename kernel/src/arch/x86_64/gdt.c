#include "gdt.h"
#include <stdint.h>

struct gdt_entry {
	uint16_t limit_low;
	uint16_t base_low;
	uint8_t  base_mid;
	uint8_t  access;
	uint8_t  granularity;
	uint8_t  base_high;
} __attribute__((packed));

struct gdt_ptr {
	uint16_t limit;
	uint64_t base;
} __attribute__((packed));

/* 7 entries: null, kcode, kdata, ucode, udata, tss_lo, tss_hi */
static struct gdt_entry gdt[7];
static struct gdt_ptr   gdtr;

static void gdt_set(int i, uint32_t base, uint32_t limit,
		    uint8_t access, uint8_t gran)
{
	gdt[i].base_low    = (uint16_t)(base & 0xFFFF);
	gdt[i].base_mid    = (uint8_t)((base >> 16) & 0xFF);
	gdt[i].base_high   = (uint8_t)((base >> 24) & 0xFF);
	gdt[i].limit_low   = (uint16_t)(limit & 0xFFFF);
	gdt[i].granularity = (uint8_t)(((limit >> 16) & 0x0F) | (gran & 0xF0));
	gdt[i].access      = access;
}

/*
 * Install a 16-byte TSS descriptor at GDT[5] + GDT[6].
 * In long mode, system descriptors are 16 bytes wide.
 */
void gdt_set_tss(uint64_t base, uint32_t limit)
{
	/* Low 8 bytes (GDT[5]) — standard TSS descriptor format */
	gdt[5].limit_low   = (uint16_t)(limit & 0xFFFF);
	gdt[5].base_low    = (uint16_t)(base & 0xFFFF);
	gdt[5].base_mid    = (uint8_t)((base >> 16) & 0xFF);
	gdt[5].access      = 0x89;  /* present, DPL=0, type=9 (64-bit TSS available) */
	gdt[5].granularity = (uint8_t)((limit >> 16) & 0x0F);
	gdt[5].base_high   = (uint8_t)((base >> 24) & 0xFF);

	/* High 8 bytes (GDT[6]) — upper 32 bits of base + reserved */
	uint32_t base_upper = (uint32_t)(base >> 32);
	uint8_t *slot6 = (uint8_t *)&gdt[6];
	slot6[0] = (uint8_t)(base_upper & 0xFF);
	slot6[1] = (uint8_t)((base_upper >> 8) & 0xFF);
	slot6[2] = (uint8_t)((base_upper >> 16) & 0xFF);
	slot6[3] = (uint8_t)((base_upper >> 24) & 0xFF);
	slot6[4] = 0;
	slot6[5] = 0;
	slot6[6] = 0;
	slot6[7] = 0;
}

void gdt_init(void)
{
	gdt_set(0, 0, 0, 0, 0);                     /* null */
	gdt_set(1, 0, 0xFFFFF, 0x9A, 0xA0);         /* kernel code 64-bit */
	gdt_set(2, 0, 0xFFFFF, 0x92, 0xC0);         /* kernel data */
	gdt_set(3, 0, 0xFFFFF, 0xF2, 0xC0);         /* user data (MUST be before user code for SYSRET) */
	gdt_set(4, 0, 0xFFFFF, 0xFA, 0xA0);         /* user code 64-bit */
	/* GDT[5]+[6] reserved for TSS — set by tss_init() */

	gdtr.limit = sizeof(gdt) - 1;
	gdtr.base  = (uint64_t)&gdt;

	__asm__ volatile(
		"lgdt %0\n\t"
		"pushq $0x08\n\t"
		"lea 1f(%%rip), %%rax\n\t"
		"pushq %%rax\n\t"
		"lretq\n\t"
		"1:\n\t"
		"mov $0x10, %%ax\n\t"
		"mov %%ax, %%ds\n\t"
		"mov %%ax, %%es\n\t"
		"mov %%ax, %%fs\n\t"
		"mov %%ax, %%gs\n\t"
		"mov %%ax, %%ss\n\t"
		:
		: "m"(gdtr)
		: "rax", "memory"
	);
}
