#include "ktest.h"
#include "mm/vmm.h"
#include "mm/pmm.h"

KTEST_REGISTER(ktest_vmm_map_unmap, "VMM map/unmap", KTEST_CAT_BOOT)
static void ktest_vmm_map_unmap(void)
{
	KTEST_BEGIN("VMM map/unmap page");

	void *phys_page = pmm_alloc_page();
	KTEST_NOT_NULL(phys_page, "alloc backing page");

	uint64_t phys = (uint64_t)phys_page - pmm_hhdm_offset();
	uint64_t vaddr = 0xFFFFC00000200000ULL;

	vmm_map_page(vaddr, phys, VMM_PRESENT | VMM_WRITE | VMM_NX);

	uint64_t resolved = vmm_virt_to_phys(vaddr);
	KTEST_EQ(resolved, phys, "mapped page resolves correctly");

	vmm_unmap_page(vaddr);

	uint64_t after = vmm_virt_to_phys(vaddr);
	KTEST_EQ(after, 0, "unmapped page resolves to 0");

	pmm_free_page(phys_page);
}
