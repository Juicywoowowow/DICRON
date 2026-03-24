#include <dicron/uaccess.h>
#include <dicron/log.h>
#include "lib/string.h"

/*
 * uaccess.c — Safe user↔kernel memory operations.
 *
 * Every syscall that takes a user pointer MUST validate it through
 * these functions before dereferencing. A malicious program could
 * otherwise read/write kernel memory.
 */

int uaccess_valid(const void *ptr, size_t len)
{
	uint64_t start = (uint64_t)ptr;
	uint64_t end = start + len;

	/* Overflow check */
	if (end < start)
		return 0;

	/* NULL pointer */
	if (start == 0 && len > 0)
		return 0;

	/* Must be entirely below kernel space */
	if (end > USER_SPACE_END)
		return 0;

	return 1;
}

int uaccess_valid_string(const char *str, size_t max_len)
{
	uint64_t addr = (uint64_t)str;

	if (addr == 0)
		return 0;
	if (addr >= USER_SPACE_END)
		return 0;

	/* Walk the string, checking each byte is in user space */
	for (size_t i = 0; i < max_len; i++) {
		if (addr + i >= USER_SPACE_END)
			return 0;
		if (str[i] == '\0')
			return 1;
	}

	/* No null terminator found within max_len */
	return 0;
}

int copy_from_user(void *dst, const void *user_src, size_t len)
{
	if (!uaccess_valid(user_src, len)) {
		klog(KLOG_ERR, "uaccess: bad copy_from_user ptr=%p len=%lu\n",
		     user_src, len);
		return -14; /* -EFAULT */
	}

	memcpy(dst, user_src, len);
	return 0;
}

int copy_to_user(void *user_dst, const void *src, size_t len)
{
	if (!uaccess_valid(user_dst, len)) {
		klog(KLOG_ERR, "uaccess: bad copy_to_user ptr=%p len=%lu\n",
		     user_dst, len);
		return -14; /* -EFAULT */
	}

	memcpy(user_dst, src, len);
	return 0;
}
