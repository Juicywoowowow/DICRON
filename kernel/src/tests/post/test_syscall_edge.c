#include "../ktest.h"
#include <dicron/syscall.h>
#include <dicron/uaccess.h>

KTEST_REGISTER(ktest_syscall_edge, "syscall: edge", KTEST_CAT_POST)
static void ktest_syscall_edge(void)
{
	KTEST_BEGIN("syscall: edge cases");

	/* Bad fd for write */
	long ret = syscall_dispatch(__NR_write, 0, 0x1000, 10, 0, 0, 0);
	KTEST_EQ(ret, -EBADF, "edge: write to fd=0 (stdin) returns EBADF");

	ret = syscall_dispatch(__NR_write, 3, 0x1000, 10, 0, 0, 0);
	KTEST_EQ(ret, -EBADF, "edge: write to fd=3 returns EBADF");

	ret = syscall_dispatch(__NR_write, -1, 0x1000, 10, 0, 0, 0);
	KTEST_EQ(ret, -EBADF, "edge: write to fd=-1 returns EBADF");

	ret = syscall_dispatch(__NR_write, 999, 0x1000, 10, 0, 0, 0);
	KTEST_EQ(ret, -EBADF, "edge: write to fd=999 returns EBADF");

	/* Write count exactly 0 */
	ret = syscall_dispatch(__NR_write, 1, 0x1000, 0, 0, 0, 0);
	KTEST_EQ(ret, 0, "edge: write count=0 returns 0");

	/* uaccess edge: zero-length at address 0 */
	int valid = uaccess_valid((void *)0, 0);
	KTEST_TRUE(valid, "edge: uaccess(NULL, 0) is valid (no bytes to access)");

	/* uaccess: max valid user address */
	valid = uaccess_valid((void *)0x00007FFFFFFFFFFEULL, 1);
	KTEST_TRUE(valid, "edge: last valid user byte accepted");

	valid = uaccess_valid((void *)0x00007FFFFFFFFFFFULL, 1);
	KTEST_TRUE(valid, "edge: USER_SPACE_END-1 accepted");

	/* uaccess: exact boundary */
	valid = uaccess_valid((void *)0x00007FFFFFFFFFFFULL, 2);
	KTEST_FALSE(valid, "edge: crossing USER_SPACE_END rejected");

	/* copy_from_user / copy_to_user with kernel addresses */
	char kbuf[16];
	int rc = copy_from_user(kbuf, (void *)0xFFFF800000001000ULL, 16);
	KTEST_EQ(rc, -14, "edge: copy_from_user rejects kernel src");

	rc = copy_to_user((void *)0xFFFF800000001000ULL, kbuf, 16);
	KTEST_EQ(rc, -14, "edge: copy_to_user rejects kernel dst");
}
