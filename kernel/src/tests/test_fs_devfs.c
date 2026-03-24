#include "ktest.h"
#include <dicron/fs.h>

static void test_devfs_console(void)
{
	struct file *f = devfs_get_console();
	KTEST_NOT_NULL(f, "devfs_get_console() returns valid file");
	if (f) {
		KTEST_NOT_NULL(f->f_op, "Console has file operations");
		KTEST_NOT_NULL(f->f_op->write, "Console has write operation");
		KTEST_EQ((f->f_inode->mode & S_IFMT), S_IFCHR, "Console is a character device");
		file_put(f);
	}
}

KTEST_REGISTER(test_devfs_console, "Devfs: console instantiation", KTEST_CAT_BOOT)
