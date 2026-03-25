#include "ktest.h"
#include <dicron/fs.h>
#include "lib/string.h"

static void test_ramfs_mkdir_write(void)
{
	/* Create a directory tree and write a file inside it */
	int ret = vfs_mkdir("/rtest", 0755);
	KTEST_EQ(ret, 0, "mkdir /rtest");

	ret = vfs_mkdir("/rtest/sub", 0755);
	KTEST_EQ(ret, 0, "mkdir /rtest/sub");

	ret = vfs_create("/rtest/sub/data.txt", 0644);
	KTEST_EQ(ret, 0, "create /rtest/sub/data.txt");

	/* Write data */
	struct inode *node = vfs_namei("/rtest/sub/data.txt");
	KTEST_NOT_NULL(node, "lookup /rtest/sub/data.txt");
	if (!node) return;
	KTEST_TRUE(S_ISREG(node->mode), "is regular file");
	KTEST_NOT_NULL(node->f_op, "has file ops");
	KTEST_NOT_NULL(node->f_op->write, "has write op");

	const char *msg = "Hello from ramfs!";
	size_t msg_len = strlen(msg);

	struct file wf = { .f_inode = node, .f_op = node->f_op, .f_pos = 0 };
	long nw = node->f_op->write(&wf, msg, msg_len);
	KTEST_EQ(nw, (long long)msg_len, "write correct bytes");

	/* Read it back */
	char buf[64];
	memset(buf, 0, sizeof(buf));
	struct file rf = { .f_inode = node, .f_op = node->f_op, .f_pos = 0 };
	long nr = node->f_op->read(&rf, buf, sizeof(buf));
	KTEST_EQ(nr, (long long)msg_len, "read correct bytes");
	KTEST_TRUE(memcmp(buf, msg, msg_len) == 0, "read-back matches");
}

KTEST_REGISTER(test_ramfs_mkdir_write, "Ramfs: mkdir + write + read-back", KTEST_CAT_BOOT)
