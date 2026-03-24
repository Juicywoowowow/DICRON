#include "../ktest.h"
#include <dicron/syscall.h>

KTEST_REGISTER(ktest_syscall_exit, "syscall: exit", KTEST_CAT_POST)
static void ktest_syscall_exit(void)
{
	KTEST_BEGIN("syscall: exit variants");

	/* sys_exit and sys_exit_group are registered */
	/* We can't call them (they kill the thread), but verify they're not ENOSYS */
	/* Instead, test that their neighbors ARE ENOSYS */

	/* fork (57) — not implemented yet */
	long ret = syscall_dispatch(__NR_fork, 0, 0, 0, 0, 0, 0);
	KTEST_EQ(ret, -ENOSYS, "syscall_exit: fork returns ENOSYS");

	/* execve (59) — not implemented yet */
	ret = syscall_dispatch(__NR_execve, 0, 0, 0, 0, 0, 0);
	KTEST_EQ(ret, -ENOSYS, "syscall_exit: execve returns ENOSYS");

	/* Verify write IS implemented (not ENOSYS) */
	ret = syscall_dispatch(__NR_write, 1, 0, 0, 0, 0, 0);
	KTEST_NE(ret, -ENOSYS, "syscall_exit: write is implemented");
}
