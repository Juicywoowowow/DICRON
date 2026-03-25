#include "ktest.h"
#include <dicron/fs.h>

static void test_vfs_deep_lookup(void)
{
	vfs_mkdir("/deep_a", 0755);
	vfs_mkdir("/deep_a/deep_b", 0755);
	vfs_mkdir("/deep_a/deep_b/deep_c", 0755);

	struct inode *c = vfs_namei("/deep_a/deep_b/deep_c");
	KTEST_NOT_NULL(c, "3-level deep lookup succeeds");
	KTEST_TRUE(S_ISDIR(c->mode), "deep node is directory");

	struct inode *miss = vfs_namei("/deep_a/deep_b/nonexist");
	KTEST_NULL(miss, "missing deep path returns NULL");
}

KTEST_REGISTER(test_vfs_deep_lookup, "VFS: deep path lookup", KTEST_CAT_BOOT)
