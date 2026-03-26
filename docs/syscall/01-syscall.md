System Calls
============

1. Overview
-----------

System calls provide the interface between userspace programs and the
kernel. DICRON implements a subset of Linux x86_64 syscalls for
compatibility with musl libc.

2. Syscall Mechanism
--------------------

System calls use the x86_64 syscall instruction:
  - Syscall number in RAX
  - Arguments in RDI, RSI, RDX, R10, R8, R9
  - Return value in RAX
  - Negative values indicate errors (errno)

3. Syscall Numbers
------------------

Implemented syscalls and their numbers:

  I/O and Filesystem:
    __NR_read       0    - Read from file descriptor
    __NR_write      1    - Write to file descriptor
    __NR_open       2    - Open file
    __NR_close      3    - Close file descriptor
    __NR_fstat      5    - Get file status
    __NR_lseek      8    - Seek file position
    __NR_readv      19   - Read from multiple buffers
    __NR_writev     20   - Write to multiple buffers
    __NR_ioctl      16   - Device control
    __NR_access     21   - Check file access permissions
    __NR_openat     257  - Open file relative to directory

  Process Management:
    __NR_exit       60   - Terminate process
    __NR_exit_group 231  - Terminate all threads
    __NR_fork       57   - Fork process
    __NR_clone       56   - Create process/thread
    __NR_execve     59   - Execute program
    __NR_getpid     39   - Get process ID

  Memory Management:
    __NR_brk        12   - Change data segment size
    __NR_mmap       9    - Map memory
    __NR_mprotect   10   - Set memory protection
    __NR_munmap     11   - Unmap memory

  Scheduling:
    __NR_sched_yield 24  - Yield CPU

  Time:
    __NR_gettimeofday 96 - Get time of day

  System:
    __NR_uname      63   - Get system information
    __NR_arch_prctl 158  - Architecture-specific settings
    __NR_set_tid_address 218 - Set thread ID address
    __NR_prlimit64  302  - Get/set resource limits

4. Implementation
-----------------

Each syscall is implemented as a function with the signature:
  long sys_<name>(long a0, long a1, long a2, long a3, long a4, long a5)

The syscall dispatcher (syscall_dispatch) validates the syscall number
and calls the appropriate handler. Handlers return negative errno values
on error, or non-negative values on success.

5. Security
-----------

The syscall layer implements security measures:
  - All user pointers are validated with uaccess functions before use
  - Bounds checking on sizes and counts
  - Invalid syscall numbers return -ENOSYS
  - Kernel pointers cannot be passed from userspace

6. Error Codes
--------------

Common error codes returned:
  ENOSYS  38  - Syscall not implemented
  EFAULT  14  - Bad address
  EINVAL  22  - Invalid argument
  ENOMEM  12  - Out of memory
  EBADF    9  - Bad file descriptor
  ENOENT   2  - No such file or directory
  ENFILE  23  - Too many open files
  EMFILE  24  - Too many open files in process
  ENOTTY  25  - Inappropriate ioctl for device

7. User Access (uaccess)
------------------------

The kernel must carefully validate and copy data to/from userspace:

  int copy_to_user(void *to, const void *from, size_t n)
      - Copy data from kernel to userspace

  int copy_from_user(void *to, const void *from, size_t n)
      - Copy data from userspace to kernel

  int probe_user_read(const void *addr, size_t n)
      - Verify userspace address is readable

  int probe_user_write(void *addr, size_t n)
      - Verify userspace address is writable
