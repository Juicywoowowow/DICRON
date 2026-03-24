#include "../ktest.h"
#include <dicron/syscall.h>
#include <dicron/time.h>
#include <dicron/io.h>

KTEST_REGISTER(ktest_syscall_stress, "syscall: dispatch stress", KTEST_CAT_POST)
static void ktest_syscall_stress(void)
{
	KTEST_BEGIN("syscall: dispatch stress");

	uint64_t t_start = ktime_ms();

	/* Hammer only write(fd=1, NULL, 0) — returns 0, no logging */
	for (int i = 0; i < 10000; i++)
		syscall_dispatch(__NR_write, 1, 0, 0, 0, 0, 0);

	uint64_t t_end = ktime_ms();
	KTEST_TRUE(1, "stress: 10000 write dispatches survived");
	kio_printf("  [PERF] 10000 syscall dispatches: %lu ms\n", (t_end - t_start));

	/* Stress bad numbers — verify no crash.
	   Use write(fd=1,NULL,0) for valid ones, only a few bad ones
	   to avoid serial spam. */
	syscall_dispatch(-1, 0, 0, 0, 0, 0, 0);
	syscall_dispatch(-999, 0, 0, 0, 0, 0, 0);
	syscall_dispatch(SYSCALL_MAX, 0, 0, 0, 0, 0, 0);
	syscall_dispatch(SYSCALL_MAX + 999, 0, 0, 0, 0, 0, 0);
	syscall_dispatch(0x7FFFFFFF, 0, 0, 0, 0, 0, 0);
	KTEST_TRUE(1, "stress: bad-number dispatches survived");
}
