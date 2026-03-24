#include "../ktest.h"
#include <dicron/syscall.h>

KTEST_REGISTER(ktest_syscall_bad_number, "syscall: bad number", KTEST_CAT_POST)
static void ktest_syscall_bad_number(void)
{
	KTEST_BEGIN("syscall: bad numbers");

	/* Negative syscall number */
	long ret = syscall_dispatch(-1, 0, 0, 0, 0, 0, 0);
	KTEST_EQ(ret, -ENOSYS, "bad_number: -1 returns ENOSYS");

	ret = syscall_dispatch(-9999, 0, 0, 0, 0, 0, 0);
	KTEST_EQ(ret, -ENOSYS, "bad_number: -9999 returns ENOSYS");

	/* Way beyond table size */
	ret = syscall_dispatch(SYSCALL_MAX, 0, 0, 0, 0, 0, 0);
	KTEST_EQ(ret, -ENOSYS, "bad_number: SYSCALL_MAX returns ENOSYS");

	ret = syscall_dispatch(SYSCALL_MAX + 100, 0, 0, 0, 0, 0, 0);
	KTEST_EQ(ret, -ENOSYS, "bad_number: MAX+100 returns ENOSYS");

	ret = syscall_dispatch(999999, 0, 0, 0, 0, 0, 0);
	KTEST_EQ(ret, -ENOSYS, "bad_number: 999999 returns ENOSYS");

	/* Unimplemented but valid-range number */
	ret = syscall_dispatch(__NR_clone, 0, 0, 0, 0, 0, 0);
	KTEST_EQ(ret, -ENOSYS, "bad_number: clone (unimpl) returns ENOSYS");

	ret = syscall_dispatch(__NR_execve, 0, 0, 0, 0, 0, 0);
	KTEST_EQ(ret, -ENOSYS, "bad_number: execve (unimpl) returns ENOSYS");

	ret = syscall_dispatch(0x7FFFFFFF, 0, 0, 0, 0, 0, 0);
	KTEST_EQ(ret, -ENOSYS, "bad_number: INT_MAX returns ENOSYS");
}
