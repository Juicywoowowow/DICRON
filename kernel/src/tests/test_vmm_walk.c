#include "ktest.h"
#include "mm/vmm.h"

KTEST_REGISTER(ktest_vmm_walk, "VMM walk", KTEST_CAT_BOOT)
static void ktest_vmm_walk(void)
{
	KTEST_BEGIN("VMM page table walk");

	/* known mapped address — our own function */
	uint64_t phys = vmm_virt_to_phys((uint64_t)&ktest_vmm_walk);
	KTEST_NE(phys, 0, "virt_to_phys on kernel text");

	/* unmapped address should return 0 */
	uint64_t bad = vmm_virt_to_phys(0xDEAD000000000000ULL);
	KTEST_EQ(bad, 0, "virt_to_phys on unmapped returns 0");
}
