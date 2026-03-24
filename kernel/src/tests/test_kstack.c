#include "ktest.h"
#include <dicron/sched.h>
#include "mm/vmm.h"

KTEST_REGISTER(ktest_kstack, "kstack alloc/free", KTEST_CAT_BOOT)
static void ktest_kstack(void)
{
	KTEST_BEGIN("guarded kernel stack");

	void *stack = kstack_alloc(4);
	KTEST_NOT_NULL(stack, "kstack_alloc(4) non-null");

	/* stack top should be mapped (last byte of mapped region) */
	uint64_t top = (uint64_t)stack;
	uint64_t phys_top = vmm_virt_to_phys(top - 1);
	KTEST_NE(phys_top, 0, "stack top page is mapped");

	/* guard page is the first page of the allocation (index 0).
	   stack_base = top - (4+1)*4096 = top - 20480.
	   Guard occupies [stack_base, stack_base + 4095].
	   Check the middle of the guard page. */
	uint64_t stack_base = top - (5 * 4096);
	uint64_t guard_addr = stack_base + 2048; /* middle of guard page */
	uint64_t phys_guard = vmm_virt_to_phys(guard_addr);
	KTEST_EQ(phys_guard, 0, "guard page is unmapped");

	kstack_free(stack, 4);
}
