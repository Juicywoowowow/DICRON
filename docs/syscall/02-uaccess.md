User Access (uaccess)
=====================

1. Overview
-----------

The uaccess subsystem provides safe transfer of data between kernel
and userspace. It validates user pointers before dereferencing to
prevent security vulnerabilities.

2. Security Model
-----------------

All pointers from userspace are considered untrusted:
  - Must verify accessibility before use
  - Must use specialized copy functions
  - Kernel pointers are rejected

3. API
-----

Validation:
  int probe_user_read(const void *addr, size_t n)
      - Check if userspace address is readable
      - Returns 0 if valid, -EFAULT otherwise

  int probe_user_write(void *addr, size_t n)
      - Check if userspace address is writable
      - Returns 0 if valid, -EFAULT otherwise

Data Transfer:
  int copy_to_user(void *to, const void *from, size_t n)
      - Copy data from kernel to userspace
      - Returns 0 on success, -EFAULT on failure

  int copy_from_user(void *to, const void *from, size_t n)
      - Copy data from userspace to kernel
      - Returns 0 on success, -EFAULT on failure

4. Implementation
-----------------

copy_to_user:
  1. Validate destination is in user range
  2. Probe write access to destination
  3. Use memcpy to copy data

copy_from_user:
  1. Validate source is in user range
  2. Probe read access to source
  3. Use memcpy to copy data

5. Error Codes
--------------

  EFAULT (14): Bad address - pointer not accessible
  - Indicates invalid userspace pointer
  - Returned to userspace as -14

6. Usage in Syscalls
-------------------

Every syscall handler must use uaccess functions:
  - Read arguments from userspace with copy_from_user
  - Write results to userspace with copy_to_user
  - Validate all pointers before use

7. Memory Regions
------------------

User space address range:
  0x0000000000000000 - 0x00007FFFFFFFFFFF

Kernel space address range:
  0xFFFF800000000000+

Pointers in kernel range from userspace are rejected.
