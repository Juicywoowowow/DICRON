#include "hpet.h"
#include "hpet_internal.h"
#include "hpet_util.h"
#include <dicron/acpi.h>
#include <dicron/log.h>
#include <dicron/panic.h>
#include <dicron/time.h>
#include <dicron/sched.h>
#include "arch/x86_64/idt.h"
#include "mm/vmm.h"
#include "limine.h"

#ifdef CONFIG_HPET

volatile uint8_t *hpet_base = NULL;
static uint64_t hpet_period_fs = 0;

/* acpi_hpet_table moved to <dicron/acpi.h> */

static volatile uint64_t ticks_1ms = 0;
static volatile uint64_t jiffies_ms = 0;

static void hpet_irq(struct idt_frame *frame)
{
	(void)frame;
	__sync_fetch_and_add(&jiffies_ms, 1);
	preempt_tick();
	
	if (hpet_base) {
		*(volatile uint64_t *)(hpet_base + HPET_REG_ISR) = 1; /* T0 */
	}
}

void hpet_init(void)
{
	struct acpi_hpet_table *hpet = acpi_find_table("HPET");
	if (!hpet) {
		klog(KLOG_WARN, "HPET: ACPI table not found, system will panic without timer.\n");
		return;
	}

	uint64_t addr = hpet->address;
	if (addr < 0xFFFF800000000000ULL) {
		uint64_t virt = addr + acpi_get_hhdm();
		/* HPET is MMIO — not covered by Limine's HHDM.  Map the page. */
		uint64_t phys_page = addr & ~0xFFFULL;
		uint64_t virt_page = virt & ~0xFFFULL;
		vmm_map_page(virt_page, phys_page, VMM_PRESENT | VMM_WRITE);
		vmm_map_page(virt_page + 4096, phys_page + 4096, VMM_PRESENT | VMM_WRITE);
		hpet_base = (volatile uint8_t *)virt;
	} else {
		hpet_base = (volatile uint8_t *)addr;
	}

	hpet_period_fs = hpet_get_period_fs();
	if (hpet_period_fs == 0) {
		klog(KLOG_ERR, "HPET: Invalid period\n");
		hpet_base = NULL;
		return;
	}

	hpet_set_main_counter(0);

	/* Route T0 to IRQ0 (Legacy Replacement Route), and global enable */
	uint64_t cfg = *(volatile uint64_t *)(hpet_base + HPET_REG_CONFIG);
	cfg |= 1; /* Global Enable */
	cfg |= 2; /* Legacy Replacement */
	*(volatile uint64_t *)(hpet_base + HPET_REG_CONFIG) = cfg;

	uint64_t fs_per_ms = 1000000000000ULL;
	ticks_1ms = fs_per_ms / hpet_period_fs;

	/* Setup Timer 0 for 1ms periodic interrupt */
	uint64_t t0_cfg = *(volatile uint64_t *)(hpet_base + HPET_REG_TIMER_BASE);
	t0_cfg |= (1 << 2); /* Interrupt Enable */
	t0_cfg |= (1 << 3); /* Periodic */
	t0_cfg |= (1 << 6); /* Timer Value Set */
	*(volatile uint64_t *)(hpet_base + HPET_REG_TIMER_BASE) = t0_cfg;
	
	*(volatile uint64_t *)(hpet_base + HPET_REG_TIMER_BASE + 8) = ticks_1ms;
	*(volatile uint64_t *)(hpet_base + HPET_REG_TIMER_BASE + 8) = ticks_1ms;

	idt_set_handler(0x20, hpet_irq);

	klog(KLOG_INFO, "HPET: Initialized (Base=0x%lx, Period=%lu fs)\n", hpet->address, hpet_period_fs);
}

int hpet_is_available(void)
{
	return hpet_base != NULL;
}

uint64_t hpet_ktime_ns(void)
{
	if (!hpet_base)
		return 0;
	
	uint64_t ticks = hpet_read_main_counter();
	return (ticks * hpet_period_fs) / 1000000ULL;
}

void hpet_ksleep_ns(uint64_t ns)
{
	if (!hpet_base || ns == 0)
		return;
	
	uint64_t start = hpet_ktime_ns();
	while (hpet_ktime_ns() - start < ns)
		__asm__ volatile("pause");
}

#endif
