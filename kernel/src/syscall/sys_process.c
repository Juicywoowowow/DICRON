#include <dicron/syscall.h>
#include <dicron/process.h>
#include <dicron/sched.h>
#include <dicron/log.h>
#include <dicron/uaccess.h>
#include <dicron/time.h>
#include "drivers/timer/rtc.h"
#include "lib/string.h"

/*
 * sys_process.c — Process/scheduler/arch syscalls for U3.
 *
 * getpid, uname, arch_prctl, set_tid_address, sched_yield
 */

/* ------------------------------------------------------------------ */
/*  sys_getpid                                                         */
/* ------------------------------------------------------------------ */

long sys_getpid(long a0, long a1, long a2, long a3, long a4, long a5)
{
	(void)a0; (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;

	struct process *proc = process_current();
	if (proc)
		return proc->pid;
	return 1; /* fallback: kernel PID */
}

/* ------------------------------------------------------------------ */
/*  sys_uname                                                          */
/* ------------------------------------------------------------------ */

/*
 * Linux utsname struct — 65-byte fields.
 * musl expects exactly this layout.
 */
#define UTSNAME_LEN 65

struct utsname {
	char sysname[UTSNAME_LEN];
	char nodename[UTSNAME_LEN];
	char release[UTSNAME_LEN];
	char version[UTSNAME_LEN];
	char machine[UTSNAME_LEN];
	char domainname[UTSNAME_LEN];
};

static void safe_copy(char *dst, const char *src, size_t max)
{
	size_t len = strlen(src);
	if (len >= max) len = max - 1;
	memcpy(dst, src, len);
	dst[len] = '\0';
}

long sys_uname(long buf_addr, long a1, long a2, long a3, long a4, long a5)
{
	(void)a1; (void)a2; (void)a3; (void)a4; (void)a5;

	if (!uaccess_valid((void *)buf_addr, sizeof(struct utsname)))
		return -EFAULT;

	struct utsname *uts = (struct utsname *)buf_addr;
	memset(uts, 0, sizeof(struct utsname));

	safe_copy(uts->sysname,    "DICRON",      UTSNAME_LEN);
	safe_copy(uts->nodename,   "dicron",      UTSNAME_LEN);
	safe_copy(uts->release,    "0.3.0",       UTSNAME_LEN);
	safe_copy(uts->version,    "#1",          UTSNAME_LEN);
	safe_copy(uts->machine,    "x86_64",      UTSNAME_LEN);
	safe_copy(uts->domainname, "(none)",      UTSNAME_LEN);

	return 0;
}

/* ------------------------------------------------------------------ */
/*  sys_arch_prctl                                                     */
/* ------------------------------------------------------------------ */

#define ARCH_SET_GS  0x1001
#define ARCH_SET_FS  0x1002
#define ARCH_GET_FS  0x1003
#define ARCH_GET_GS  0x1004

#define MSR_FS_BASE  0xC0000100
#define MSR_GS_BASE  0xC0000101

static inline void wrmsr(uint32_t msr, uint64_t value)
{
	uint32_t lo = (uint32_t)(value & 0xFFFFFFFF);
	uint32_t hi = (uint32_t)(value >> 32);
	__asm__ volatile("wrmsr" : : "c"(msr), "a"(lo), "d"(hi));
}

static inline uint64_t rdmsr(uint32_t msr)
{
	uint32_t lo, hi;
	__asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
	return ((uint64_t)hi << 32) | lo;
}

long sys_arch_prctl(long code, long addr, long a2,
		    long a3, long a4, long a5)
{
	(void)a2; (void)a3; (void)a4; (void)a5;

	switch (code) {
	case ARCH_SET_FS:
		wrmsr(MSR_FS_BASE, (uint64_t)addr);
		return 0;

	case ARCH_SET_GS:
		wrmsr(MSR_GS_BASE, (uint64_t)addr);
		return 0;

	case ARCH_GET_FS:
		if (!uaccess_valid((void *)addr, sizeof(uint64_t)))
			return -EFAULT;
		*(uint64_t *)addr = rdmsr(MSR_FS_BASE);
		return 0;

	case ARCH_GET_GS:
		if (!uaccess_valid((void *)addr, sizeof(uint64_t)))
			return -EFAULT;
		*(uint64_t *)addr = rdmsr(MSR_GS_BASE);
		return 0;

	default:
		return -EINVAL;
	}
}

/* ------------------------------------------------------------------ */
/*  sys_set_tid_address                                                */
/* ------------------------------------------------------------------ */

long sys_set_tid_address(long tidptr, long a1, long a2,
			 long a3, long a4, long a5)
{
	(void)tidptr; (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;

	/* musl calls this early.  Just return the current TID (= PID). */
	struct process *proc = process_current();
	return proc ? proc->pid : 1;
}

/* ------------------------------------------------------------------ */
/*  sys_sched_yield                                                    */
/* ------------------------------------------------------------------ */

long sys_sched_yield(long a0, long a1, long a2,
		     long a3, long a4, long a5)
{
	(void)a0; (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;

	kthread_yield();
	return 0;
}

struct timeval {
	long tv_sec;
	long tv_usec;
};

long sys_gettimeofday(long tv_addr, long tz_addr, long a2, long a3, long a4, long a5)
{
	(void)tz_addr; (void)a2; (void)a3; (void)a4; (void)a5;
	if (tv_addr) {
		if (!uaccess_valid((void *)tv_addr, sizeof(struct timeval)))
			return -EFAULT;
		struct timeval *tv = (struct timeval *)tv_addr;
		tv->tv_sec = (long)rtc_unix_time();
		tv->tv_usec = (long)((ktime_ms() % 1000) * 1000);
	}
	return 0;
}

long sys_prlimit64(long pid, long resource, long new_limit, long old_limit, long a4, long a5)
{
	(void)pid; (void)resource; (void)new_limit; (void)a4; (void)a5;
	if (old_limit) {
		/* Return 0 limits. Musl uses this for stack size, file count limits */
		if (!uaccess_valid((void *)old_limit, 16)) /* rlimit64 struct is 16 bytes */
			return -EFAULT;
		/* rlimit64: uint64_t cur, uint64_t max */
		uint64_t *limits = (uint64_t *)old_limit;
		limits[0] = 0; /* soft limit */
		limits[1] = 0; /* hard limit */
	}
	return 0;
}

/* ------------------------------------------------------------------ */
/*  sys_times                                                          */
/* ------------------------------------------------------------------ */

/* Linux tms struct — all fields in clock ticks (HZ=100) */
struct tms {
	long tms_utime;  /* user time */
	long tms_stime;  /* system time */
	long tms_cutime; /* children user time */
	long tms_cstime; /* children system time */
};

#define HZ 100  /* kernel clock ticks per second */

long sys_times(long buf_addr, long a1, long a2, long a3, long a4, long a5)
{
	(void)a1; (void)a2; (void)a3; (void)a4; (void)a5;

	/* ticks = ms / (1000 / HZ) = ms * HZ / 1000 */
	long ticks = (long)((ktime_ms() * HZ) / 1000);

	if (buf_addr) {
		if (!uaccess_valid((void *)buf_addr, sizeof(struct tms)))
			return -EFAULT;
		struct tms *t = (struct tms *)buf_addr;
		t->tms_utime  = ticks;
		t->tms_stime  = 0;
		t->tms_cutime = 0;
		t->tms_cstime = 0;
	}
	return ticks; /* return value is elapsed real time in ticks */
}

/* ------------------------------------------------------------------ */
/*  sys_clock_gettime                                                  */
/* ------------------------------------------------------------------ */

/* POSIX clockid values */
#define CLOCK_REALTIME   0
#define CLOCK_MONOTONIC  1
#define CLOCK_PROCESS_CPUTIME_ID 2

struct timespec {
	long tv_sec;
	long tv_nsec;
};

long sys_clock_gettime(long clkid, long ts_addr, long a2,
		       long a3, long a4, long a5)
{
	(void)a2; (void)a3; (void)a4; (void)a5;

	if (!uaccess_valid((void *)ts_addr, sizeof(struct timespec)))
		return -EFAULT;

	struct timespec *ts = (struct timespec *)ts_addr;
	uint64_t ms;

	switch (clkid) {
	case CLOCK_REALTIME:
		ts->tv_sec  = (long)rtc_unix_time();
		ts->tv_nsec = (long)((ktime_ms() % 1000) * 1000000);
		break;
	case CLOCK_MONOTONIC:
	case CLOCK_PROCESS_CPUTIME_ID:
		ms = ktime_ms();
		ts->tv_sec  = (long)(ms / 1000);
		ts->tv_nsec = (long)((ms % 1000) * 1000000);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}
