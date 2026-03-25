#include "ktest.h"
#include <dicron/fs.h>

static void test_vfs_nested_mkdir(void)
{
	vfs_mkdir("/nestpar", 0755);
	int ret = vfs_mkdir("/nestpar/child", 0755);
	KTEST_EQ(ret, 0, "nested mkdir succeeds");

	struct inode *child = vfs_namei("/nestpar/child");
	KTEST_NOT_NULL(child, "nested dir exists");
	KTEST_TRUE(S_ISDIR(child->mode), "nested dir is S_IFDIR");
}

KTEST_REGISTER(test_vfs_nested_mkdir, "VFS: nested mkdir", KTEST_CAT_BOOT)
