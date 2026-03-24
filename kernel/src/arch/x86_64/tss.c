#include "tss.h"
#include "gdt.h"
#include "lib/string.h"
#include <stdint.h>

/*
 * tss.c — Task State Segment for x86_64.
 *
 * The TSS in long mode is only used for:
 *   1. RSP0 — the kernel stack pointer loaded on ring 3→0 transition
 *   2. IST entries — interrupt stack pointers for specific vectors
 *
 * We install it as a system descriptor in the GDT (16 bytes in long mode)
 * and load it with LTR.
 */

static struct tss tss __attribute__((aligned(16)));

/*
 * Install the TSS descriptor into GDT slots 5 and 6 (16-byte system descriptor).
 * TSS selector = 5 * 8 = 0x28.
 */
static void tss_install_gdt(void)
{
	uint64_t base = (uint64_t)&tss;
	uint32_t limit = sizeof(struct tss) - 1;

	/*
	 * In long mode, a TSS descriptor is 16 bytes spanning two GDT slots.
	 * We write raw bytes into the GDT at the location exported by gdt.h.
	 */
	extern void gdt_set_tss(uint64_t base, uint32_t limit);
	gdt_set_tss(base, limit);
}

void tss_init(void)
{
	memset(&tss, 0, sizeof(tss));
	tss.iopb_offset = sizeof(struct tss); /* no I/O permission bitmap */

	tss_install_gdt();

	/* Load TSS — selector 0x28 (GDT index 5) */
	__asm__ volatile("ltr %w0" : : "r"((uint16_t)0x28));
}

void tss_set_rsp0(uint64_t rsp0)
{
	tss.rsp0 = rsp0;
}
