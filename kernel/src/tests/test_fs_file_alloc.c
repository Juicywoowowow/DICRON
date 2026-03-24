#include "ktest.h"
#include <dicron/fs.h>

static void test_fd_management(void)
{
	struct file *f = file_alloc();
	KTEST_NOT_NULL(f, "file_alloc() works independently of process");
	file_put(f);
}

KTEST_REGISTER(test_fd_management, "VFS: struct file allocation", KTEST_CAT_BOOT)
