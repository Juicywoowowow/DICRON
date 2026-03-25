#include "ktest.h"
#include <dicron/fs.h>

static void test_vfs_create_in_subdir(void)
{
	vfs_mkdir("/subcreat", 0755);
	int ret = vfs_create("/subcreat/hello.txt", 0644);
	KTEST_EQ(ret, 0, "create file in subdir succeeds");

	struct inode *f = vfs_namei("/subcreat/hello.txt");
	KTEST_NOT_NULL(f, "file in subdir is findable");
	KTEST_TRUE(S_ISREG(f->mode), "file in subdir is S_IFREG");
}

KTEST_REGISTER(test_vfs_create_in_subdir, "VFS: create file in subdir", KTEST_CAT_BOOT)
