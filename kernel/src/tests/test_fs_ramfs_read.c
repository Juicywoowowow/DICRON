#include "ktest.h"
#include <dicron/fs.h>
#include "lib/string.h"

static void test_ramfs_read(void)
{
	struct inode *root = vfs_namei("/");
	if (root && root->i_op && root->i_op->create) {
		root->i_op->create(root, "readtest.txt", 0666);
	}
	
	struct inode *inode = vfs_namei("/readtest.txt");
	if (!inode) return;
	
	struct file *f = file_alloc();
	KTEST_NOT_NULL(f, "Allocated file struct for read test");
	if (!f) return;
	
	f->f_inode = inode;
	f->f_op = inode->f_op;
	f->f_mode = O_RDWR;
	f->f_pos = 0;
	
	const char *msg = "hello filesystem test";
	if (f->f_op && f->f_op->write) {
		f->f_op->write(f, msg, strlen(msg));
	}
	f->f_pos = 0; /* Reset for read */
	
	char buf[32];
	memset(buf, 0, sizeof(buf));
	
	long rret = f->f_op->read(f, buf, sizeof(buf));
	KTEST_EQ(rret, (long)strlen(msg), "Read returns correct byte count");
	
	int cmp = memcmp(buf, msg, strlen(msg));
	KTEST_EQ(cmp, 0, "Read data matches written data");
	
	file_put(f);
}

KTEST_REGISTER(test_ramfs_read, "Ramfs: read test", KTEST_CAT_BOOT)
