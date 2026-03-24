#ifndef _DICRON_SYSCALL_H
#define _DICRON_SYSCALL_H

/* Linux x86_64 syscall numbers — 1:1 match */
#define __NR_read            0
#define __NR_write           1
#define __NR_open            2
#define __NR_close           3
#define __NR_fstat           5
#define __NR_lseek           8
#define __NR_mmap            9
#define __NR_mprotect       10
#define __NR_munmap         11
#define __NR_brk            12
#define __NR_rt_sigaction   13
#define __NR_rt_sigprocmask 14
#define __NR_ioctl          16
#define __NR_readv          19
#define __NR_writev         20
#define __NR_access         21
#define __NR_sched_yield    24
#define __NR_dup2           33
#define __NR_getpid         39
#define __NR_clone          56
#define __NR_fork           57
#define __NR_execve         59
#define __NR_exit           60
#define __NR_uname          63
#define __NR_getcwd         79
#define __NR_gettimeofday   96
#define __NR_arch_prctl    158
#define __NR_set_tid_address 218
#define __NR_exit_group    231
#define __NR_openat        257
#define __NR_prlimit64     302

#define SYSCALL_MAX        512

/* Return codes */
#define ENOSYS  38
#define EFAULT  14
#define EINVAL  22
#define ENOMEM  12
#define EBADF    9
#define ENOENT   2
#define ENFILE  23
#define EMFILE  24
#define ENOTTY  25

void syscall_init(void);
void syscall_table_init(void);
long syscall_dispatch(long nr, long a0, long a1, long a2, long a3, long a4, long a5);

#endif
