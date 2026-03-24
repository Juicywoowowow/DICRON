#include "ktest.h"
#include <dicron/fs.h>
#include "lib/string.h"

static void test_ramfs_write(void)
{
	struct inode *root = vfs_namei("/");
	if (root && root->i_op && root->i_op->create) {
		root->i_op->create(root, "ktest.txt", 0666);
	}
	
	struct inode *inode = vfs_namei("/ktest.txt");
	if (!inode) return;
	
	struct file *f = file_alloc();
	KTEST_NOT_NULL(f, "Allocated file struct for write test");
	if (!f) return;
	
	f->f_inode = inode;
	f->f_op = inode->f_op;
	f->f_mode = O_RDWR;
	f->f_pos = 0;
	
	const char *msg = "hello filesystem test";
	long wret = f->f_op->write(f, msg, strlen(msg));
	KTEST_EQ(wret, (long)strlen(msg), "Write returns correct byte count");
	KTEST_EQ(f->f_pos, strlen(msg), "File position advanced after write");
	KTEST_EQ(inode->size, strlen(msg), "Inode size updated after write");
	
	file_put(f);
}

KTEST_REGISTER(test_ramfs_write, "Ramfs: write test", KTEST_CAT_BOOT)
