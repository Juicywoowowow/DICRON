#include "ktest.h"
#include <dicron/fs.h>

static void test_vfs_lookup_fail(void)
{
	struct inode *enoent = vfs_namei("/does_not_exist.txt");
	KTEST_NULL(enoent, "vfs_namei on non-existent file returns NULL");
}

KTEST_REGISTER(test_vfs_lookup_fail, "VFS: lookup non-existent", KTEST_CAT_BOOT)
