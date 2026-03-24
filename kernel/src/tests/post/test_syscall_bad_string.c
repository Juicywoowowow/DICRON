#include "../ktest.h"
#include <dicron/uaccess.h>

KTEST_REGISTER(ktest_syscall_bad_string, "syscall: bad string", KTEST_CAT_POST)
static void ktest_syscall_bad_string(void)
{
	KTEST_BEGIN("syscall: bad string validation");

	/* NULL string */
	int valid = uaccess_valid_string(NULL, 256);
	KTEST_FALSE(valid, "bad_str: NULL rejected");

	/* Kernel address string */
	valid = uaccess_valid_string((const char *)0xFFFF800000000000ULL, 256);
	KTEST_FALSE(valid, "bad_str: kernel addr rejected");

	/* Zero max_len — can't find null terminator in 0 bytes */
	valid = uaccess_valid_string((const char *)0x1000, 0);
	KTEST_FALSE(valid, "bad_str: zero max_len returns false");

	/* Range validation only (no dereference) */
	valid = uaccess_valid((void *)0x00007FFFFFFFFFFEULL, 2);
	KTEST_TRUE(valid, "bad_str: 2 bytes at near-boundary (last 2 user bytes)");

	/* This one actually crosses */
	valid = uaccess_valid((void *)0x00007FFFFFFFFFFFULL, 2);
	KTEST_FALSE(valid, "bad_str: 2 bytes crossing USER_SPACE_END rejected");
}
