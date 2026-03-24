#include "ktest.h"
#include <dicron/fs.h>

static void test_vfs_root(void)
{
	struct inode *root = vfs_namei("/");
	KTEST_NOT_NULL(root, "vfs_namei('/') returns root inode");
	if (root) {
		KTEST_TRUE(S_ISDIR(root->mode), "Root inode is a directory");
	}
}

KTEST_REGISTER(test_vfs_root, "VFS: root mount", KTEST_CAT_BOOT)
