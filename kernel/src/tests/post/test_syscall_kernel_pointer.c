#include "../ktest.h"
#include <dicron/syscall.h>
#include <dicron/uaccess.h>

KTEST_REGISTER(ktest_syscall_kernel_pointer, "syscall: kernel pointer", KTEST_CAT_POST)
static void ktest_syscall_kernel_pointer(void)
{
	KTEST_BEGIN("syscall: kernel pointer rejection");

	/* Kernel addresses must NEVER be accepted by uaccess */

	/* Typical kernel higher-half address */
	int valid = uaccess_valid((void *)0xFFFF800000000000ULL, 1);
	KTEST_FALSE(valid, "kern_ptr: HHDM base rejected");

	valid = uaccess_valid((void *)0xFFFFFFFF80000000ULL, 1);
	KTEST_FALSE(valid, "kern_ptr: kernel text rejected");

	valid = uaccess_valid((void *)0xFFFFC00000000000ULL, 4096);
	KTEST_FALSE(valid, "kern_ptr: kstack region rejected");

	/* Just at the boundary */
	valid = uaccess_valid((void *)0x0000800000000000ULL, 1);
	KTEST_FALSE(valid, "kern_ptr: USER_SPACE_END boundary rejected");

	valid = uaccess_valid((void *)0x00007FFFFFFFF000ULL, 0x2000);
	KTEST_FALSE(valid, "kern_ptr: crosses USER_SPACE_END rejected");

	/* Valid user address for comparison */
	valid = uaccess_valid((void *)0x0000000000400000ULL, 4096);
	KTEST_TRUE(valid, "kern_ptr: typical user addr accepted");

	valid = uaccess_valid((void *)0x00007FFFFFFFEFF0ULL, 16);
	KTEST_TRUE(valid, "kern_ptr: high user stack addr accepted");

	/* sys_write with kernel buffer */
	long ret = syscall_dispatch(__NR_write, 1,
				    (long)0xFFFF800000001000ULL, 10, 0, 0, 0);
	KTEST_EQ(ret, -EFAULT, "kern_ptr: sys_write rejects kernel buffer");
}
