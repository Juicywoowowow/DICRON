#include "../ktest.h"
#include <dicron/syscall.h>
#include <dicron/uaccess.h>

KTEST_REGISTER(ktest_syscall_bad_pointer, "syscall: bad pointer", KTEST_CAT_POST)
static void ktest_syscall_bad_pointer(void)
{
	KTEST_BEGIN("syscall: bad user pointers");

	/* NULL pointer with non-zero count */
	long ret = syscall_dispatch(__NR_write, 1, 0, 100, 0, 0, 0);
	KTEST_EQ(ret, -EFAULT, "bad_ptr: NULL buf with count=100");

	/* Overflow: ptr + count wraps around */
	ret = syscall_dispatch(__NR_write, 1,
			       (long)0x7FFFFFFFFFFFF000ULL, 0x2000, 0, 0, 0);
	KTEST_EQ(ret, -EFAULT, "bad_ptr: overflow wrapping rejected");

	/* Negative count */
	ret = syscall_dispatch(__NR_write, 1, 0x1000, -1, 0, 0, 0);
	KTEST_EQ(ret, -EINVAL, "bad_ptr: negative count rejected");

	/* Zero count with NULL — should return 0, not fault */
	ret = syscall_dispatch(__NR_write, 1, 0, 0, 0, 0, 0);
	KTEST_EQ(ret, 0, "bad_ptr: zero count with NULL returns 0");

	/* Range validation only (no dereference) */
	int valid = uaccess_valid((void *)0x1, 10);
	KTEST_TRUE(valid, "bad_ptr: addr 0x1 passes range check");

	valid = uaccess_valid((void *)0x0, 10);
	KTEST_FALSE(valid, "bad_ptr: NULL with len>0 fails range check");
}
