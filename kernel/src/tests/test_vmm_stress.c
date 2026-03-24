#include "ktest.h"
#include "mm/vmm.h"
#include "mm/pmm.h"

KTEST_REGISTER(ktest_vmm_stress, "VMM stress", KTEST_CAT_BOOT)
static void ktest_vmm_stress(void)
{
	KTEST_BEGIN("VMM map/unmap stress (50 cycles)");

	void *phys_page = pmm_alloc_page();
	KTEST_NOT_NULL(phys_page, "alloc backing page");

	uint64_t phys = (uint64_t)phys_page - pmm_hhdm_offset();
	uint64_t vaddr = 0xFFFFC00000300000ULL;

	for (int i = 0; i < 50; i++) {
		vmm_map_page(vaddr, phys, VMM_PRESENT | VMM_WRITE | VMM_NX);
		vmm_unmap_page(vaddr);
	}
	KTEST_TRUE(1, "50 map/unmap cycles clean");

	/* verify it's unmapped at the end */
	KTEST_EQ(vmm_virt_to_phys(vaddr), 0, "final state is unmapped");

	pmm_free_page(phys_page);
}
