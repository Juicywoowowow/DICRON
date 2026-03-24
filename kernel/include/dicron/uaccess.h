#ifndef _DICRON_UACCESS_H
#define _DICRON_UACCESS_H

#include <stddef.h>
#include <stdint.h>

/*
 * User address space boundaries.
 * Anything at or above USER_SPACE_END is kernel memory.
 * User pointers must fall entirely within [0, USER_SPACE_END).
 */
#define USER_SPACE_END  0x0000800000000000ULL

/* Validate that [ptr, ptr+len) is entirely within user address space */
int uaccess_valid(const void *ptr, size_t len);

/* Validate a user string: must be in user space and null-terminated within max_len */
int uaccess_valid_string(const char *str, size_t max_len);

/* Safe copy from user to kernel. Returns 0 on success, -EFAULT on bad pointer. */
int copy_from_user(void *dst, const void *user_src, size_t len);

/* Safe copy from kernel to user. Returns 0 on success, -EFAULT on bad pointer. */
int copy_to_user(void *user_dst, const void *src, size_t len);

#endif
