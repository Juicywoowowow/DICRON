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

struct acpi_hpet_table {
	char signature[4];
	uint32_t length;
	uint8_t revision;
	uint8_t checksum;
	char oem_id[6];
	char oem_table_id[8];
	uint32_t oem_revision;
	uint32_t creator_id;
	uint32_t creator_revision;
	uint32_t event_timer_block_id;
	uint8_t base_address_id;
	uint8_t register_bit_width;
	uint8_t register_bit_offset;
	uint8_t access_width;
	uint64_t address;
	uint8_t hpet_number;
	uint16_t minimum_tick;
	uint8_t page_protection;
} __attribute__((packed));

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

uint64_t ktime_ns(void)
{
	if (!hpet_base)
		return 0;
	
	uint64_t ticks = hpet_read_main_counter();
	return (ticks * hpet_period_fs) / 1000000ULL;
}

void ksleep_ns(uint64_t ns)
{
	if (!hpet_base || ns == 0)
		return;
	
	uint64_t start = ktime_ns();
	while (ktime_ns() - start < ns)
		__asm__ volatile("pause");
}

#ifndef CONFIG_PIT
uint64_t ktime_ms(void)
{
	if (hpet_base)
		return jiffies_ms;
	return 0;
}

void ksleep_ms(uint32_t ms)
{
	if (hpet_base) {
		uint64_t target = jiffies_ms + ms;
		while (jiffies_ms < target)
			__asm__ volatile("hlt");
	}
}
#endif

#endif
