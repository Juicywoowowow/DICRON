#ifndef _DICRON_ARCH_X86_64_GDT_H
#define _DICRON_ARCH_X86_64_GDT_H

#include <stdint.h>

/* Segment selectors
 * SYSRET requires: user_data at GDT[3], user_code at GDT[4]
 * Because SYSRET in 64-bit mode loads CS = STAR[63:48]+16, SS = STAR[63:48]+8
 */
#define GDT_KERNEL_CS  0x08
#define GDT_KERNEL_DS  0x10
#define GDT_USER_DS    0x1B   /* index 3, RPL 3 — must be before user CS for SYSRET */
#define GDT_USER_CS    0x23   /* index 4, RPL 3 */
#define GDT_TSS_SEL    0x28   /* index 5 (16-byte descriptor spans 5+6) */

void gdt_init(void);
void gdt_set_tss(uint64_t base, uint32_t limit);

#endif
