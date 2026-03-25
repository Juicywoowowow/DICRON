#include "ktest.h"
#include <dicron/fs.h>

static void test_vfs_mkdir(void)
{
	int ret = vfs_mkdir("/testdir", 0755);
	KTEST_EQ(ret, 0, "vfs_mkdir('/testdir') succeeds");

	struct inode *dir = vfs_namei("/testdir");
	KTEST_NOT_NULL(dir, "'/testdir' exists after mkdir");
	KTEST_TRUE(S_ISDIR(dir->mode), "'/testdir' is a directory");
}

KTEST_REGISTER(test_vfs_mkdir, "VFS: mkdir basic", KTEST_CAT_BOOT)
