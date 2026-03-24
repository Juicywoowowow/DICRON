#include "arch/x86_64/gdt.h"
#include "arch/x86_64/tss.h"
#include <dicron/log.h>
#include <stdint.h>

/*
 * syscall_setup.c — Configure SYSCALL/SYSRET MSRs for x86_64.
 */

#define MSR_EFER   0xC0000080
#define MSR_STAR   0xC0000081
#define MSR_LSTAR  0xC0000082
#define MSR_SFMASK 0xC0000084
#define EFER_SCE   (1ULL << 0)

static inline void wrmsr(uint32_t msr, uint64_t value)
{
	uint32_t lo = (uint32_t)(value & 0xFFFFFFFF);
	uint32_t hi = (uint32_t)(value >> 32);
	__asm__ volatile("wrmsr" : : "c"(msr), "a"(lo), "d"(hi));
}

static inline uint64_t rdmsr(uint32_t msr)
{
	uint32_t lo, hi;
	__asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
	return ((uint64_t)hi << 32) | lo;
}

extern void syscall_entry(void);

void syscall_init(void)
{
	/* Enable syscall extensions in EFER */
	uint64_t efer = rdmsr(MSR_EFER);
	efer |= EFER_SCE;
	wrmsr(MSR_EFER, efer);

	/*
	 * STAR layout:
	 *   [47:32] = kernel CS selector (0x08). SYSCALL loads CS=0x08, SS=0x10.
	 *   [63:48] = user base selector (0x10). SYSRET loads SS=base+8=0x18|3, CS=base+16=0x20|3.
	 *   GDT[3]=user_data(0x18), GDT[4]=user_code(0x20) — matches SYSRET requirements.
	 */
	uint64_t star = ((uint64_t)0x0010 << 48) | ((uint64_t)0x0008 << 32);
	wrmsr(MSR_STAR, star);

	/* LSTAR = kernel entry point for SYSCALL */
	wrmsr(MSR_LSTAR, (uint64_t)syscall_entry);

	/* SFMASK = mask IF on entry (disable interrupts until we're on the kernel stack) */
	wrmsr(MSR_SFMASK, 0x200);

	klog(KLOG_INFO, "SYSCALL/SYSRET configured (LSTAR=%p)\n", (void *)syscall_entry);
}
