#include "ktest.h"
#include <dicron/fs.h>

static void test_ramfs_create(void)
{
	struct inode *root = vfs_namei("/");
	KTEST_NOT_NULL(root, "Root inode exists for file creation");
	if (!root) return;
	
	KTEST_NOT_NULL(root->i_op, "Root inode has operations");
	KTEST_NOT_NULL(root->i_op->create, "Root inode has create function");
	
	int ret = root->i_op->create(root, "ktest.txt", 0666);
	KTEST_EQ(ret, 0, "create('ktest.txt') succeeds");
}

KTEST_REGISTER(test_ramfs_create, "Ramfs: create file", KTEST_CAT_BOOT)
