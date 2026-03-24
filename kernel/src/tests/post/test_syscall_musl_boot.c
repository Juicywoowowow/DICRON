#include "../ktest.h"
#include <dicron/syscall.h>

extern long syscall_dispatch(long nr, long a0, long a1, long a2,
			     long a3, long a4, long a5);

static void test_gettimeofday(void)
{
	/* gettimeofday with NULL pointers should succeed (return 0) */
	long ret = syscall_dispatch(__NR_gettimeofday, 0, 0, 0, 0, 0, 0);
	KTEST_EQ(ret, 0, "gettimeofday(NULL, NULL) returns 0");
	
	/* gettimeofday with bad pointer should return -EFAULT */
	ret = syscall_dispatch(__NR_gettimeofday, -4096L, 0, 0, 0, 0, 0);
	KTEST_EQ(ret, -EFAULT, "gettimeofday(bad_ptr) returns -EFAULT");
}

static void test_prlimit64(void)
{
	/* prlimit64 with NULL limits should return 0 */
	long ret = syscall_dispatch(__NR_prlimit64, 0, 0, 0, 0, 0, 0);
	KTEST_EQ(ret, 0, "prlimit64(..., NULL, NULL) returns 0");
	
	/* prlimit64 with bad pointer should return -EFAULT */
	ret = syscall_dispatch(__NR_prlimit64, 0, 0, 0, -4096L, 0, 0);
	KTEST_EQ(ret, -EFAULT, "prlimit64(..., old_limit=bad_ptr) returns -EFAULT");
}

static void test_ioctl(void)
{
	/* ioctl on a bad FD (or in boot thread context without FDs) should return -EBADF */
	long ret = syscall_dispatch(__NR_ioctl, 1, 0, 0, 0, 0, 0);
	KTEST_EQ(ret, -EBADF, "ioctl(1, ...) without fd returns -EBADF");
}

static void test_writev(void)
{
	/* writev with bad iovec pointer returns -EFAULT */
	long ret = syscall_dispatch(__NR_writev, 1, -4096L, 1, 0, 0, 0);
	KTEST_EQ(ret, -EFAULT, "writev(1, bad_ptr, 1) returns -EFAULT");
	
	/* writev with valid iovec but over maximum count (1024) returns -EINVAL */
	ret = syscall_dispatch(__NR_writev, 1, 0, 2048, 0, 0, 0);
	KTEST_EQ(ret, -EINVAL, "writev(..., 2048) returns -EINVAL");
}

static void test_readv(void)
{
	/* readv with negative iovcnt returns -EINVAL */
	long ret = syscall_dispatch(__NR_readv, 1, 0, -5, 0, 0, 0);
	KTEST_EQ(ret, -EINVAL, "readv(..., -5) returns -EINVAL");
	
	/* readv with iovcnt = 0 returns 0 */
	ret = syscall_dispatch(__NR_readv, 1, 0, 0, 0, 0, 0);
	KTEST_EQ(ret, 0, "readv(..., 0) returns 0");
}

KTEST_REGISTER(test_gettimeofday, "musl_boot: gettimeofday", KTEST_CAT_POST)
KTEST_REGISTER(test_prlimit64, "musl_boot: prlimit64", KTEST_CAT_POST)
KTEST_REGISTER(test_ioctl, "musl_boot: ioctl stub", KTEST_CAT_POST)
KTEST_REGISTER(test_writev, "musl_boot: writev validation", KTEST_CAT_POST)
KTEST_REGISTER(test_readv, "musl_boot: readv validation", KTEST_CAT_POST)
