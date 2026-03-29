#include <dicron/syscall.h>
#include <dicron/uaccess.h>
#include <dicron/log.h>
#include <dicron/io.h>
#include <dicron/sched.h>

/*
 * syscall.c — Secure syscall dispatch.
 *
 * Security rules:
 *   1. Every user pointer is validated via uaccess before dereferencing
 *   2. Counts/sizes are bounds-checked against sane limits
 *   3. Unknown syscall numbers return -ENOSYS
 *   4. All args are treated as untrusted
 */

typedef long (*syscall_fn_t)(long, long, long, long, long, long);

static syscall_fn_t syscall_table[SYSCALL_MAX];

extern long sys_read(long, long, long, long, long, long);
extern long sys_write(long, long, long, long, long, long);
extern long sys_open(long, long, long, long, long, long);
extern long sys_close(long, long, long, long, long, long);

extern long sys_readv(long, long, long, long, long, long);
extern long sys_writev(long, long, long, long, long, long);
extern long sys_ioctl(long, long, long, long, long, long);
extern long sys_openat(long, long, long, long, long, long);
extern long sys_gettimeofday(long, long, long, long, long, long);
extern long sys_prlimit64(long, long, long, long, long, long);
extern long sys_times(long, long, long, long, long, long);
extern long sys_clock_gettime(long, long, long, long, long, long);

/* --- sys_exit --- */
static long sys_exit(long code, long a1, long a2,
		     long a3, long a4, long a5)
{
	(void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
	klog(KLOG_INFO, "process exited with code %ld\n", code);
	kthread_exit();
	return 0;
}

/* --- sys_exit_group --- */
static long sys_exit_group(long code, long a1, long a2,
			   long a3, long a4, long a5)
{
	return sys_exit(code, a1, a2, a3, a4, a5);
}

/* --- External syscall handlers (sys_memory.c, sys_process.c) --- */
extern long sys_brk(long, long, long, long, long, long);
extern long sys_mmap(long, long, long, long, long, long);
extern long sys_munmap(long, long, long, long, long, long);
extern long sys_getpid(long, long, long, long, long, long);
extern long sys_uname(long, long, long, long, long, long);
extern long sys_arch_prctl(long, long, long, long, long, long);
extern long sys_set_tid_address(long, long, long, long, long, long);
extern long sys_sched_yield(long, long, long, long, long, long);

/* --- Registration --- */

static void register_syscall(int nr, syscall_fn_t fn)
{
	if (nr >= 0 && nr < SYSCALL_MAX)
		syscall_table[nr] = fn;
}

void syscall_table_init(void)
{
	for (int i = 0; i < SYSCALL_MAX; i++)
		syscall_table[i] = 0;

	/* I/O and FS */
	register_syscall(__NR_read,            sys_read);
	register_syscall(__NR_write,           sys_write);
	register_syscall(__NR_open,            sys_open);
	register_syscall(__NR_close,           sys_close);
	register_syscall(__NR_ioctl,           sys_ioctl);
	register_syscall(__NR_readv,           sys_readv);
	register_syscall(__NR_writev,          sys_writev);
	register_syscall(__NR_openat,          sys_openat);

	/* Process */
	register_syscall(__NR_exit,            sys_exit);
	register_syscall(__NR_exit_group,      sys_exit_group);
	register_syscall(__NR_getpid,          sys_getpid);

	/* Scheduler */
	register_syscall(__NR_sched_yield,     sys_sched_yield);

	/* Memory */
	register_syscall(__NR_brk,             sys_brk);
	register_syscall(__NR_mmap,            sys_mmap);
	register_syscall(__NR_munmap,          sys_munmap);

	/* Info / Arch */
	register_syscall(__NR_uname,           sys_uname);
	register_syscall(__NR_arch_prctl,      sys_arch_prctl);
	register_syscall(__NR_set_tid_address, sys_set_tid_address);
	register_syscall(__NR_gettimeofday,    sys_gettimeofday);
	register_syscall(__NR_prlimit64,       sys_prlimit64);
	register_syscall(100 /* times */,      sys_times);
	register_syscall(228 /* clock_gettime */, sys_clock_gettime);
}

/* --- Dispatch --- */

long syscall_dispatch(long nr, long a0, long a1, long a2,
		      long a3, long a4, long a5)
{
	/* Bounds check */
	if (nr < 0 || nr >= SYSCALL_MAX) {
		klog(KLOG_WARN, "syscall: out-of-bounds nr=%ld\n", nr);
		return -ENOSYS;
	}

	/* Lookup handler */
	syscall_fn_t fn = syscall_table[nr];
	if (!fn) {
		klog(KLOG_WARN, "syscall: unimplemented nr=%ld\n", nr);
		return -ENOSYS;
	}

	return fn(a0, a1, a2, a3, a4, a5);
}
