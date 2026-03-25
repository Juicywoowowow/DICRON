#include "ktest.h"
#include <dicron/fs.h>
#include <dicron/cpio.h>
#include "lib/string.h"

/* Build a minimal cpio newc header */
static void write_hex8(char *dst, uint32_t val)
{
	for (int i = 7; i >= 0; i--) {
		uint32_t nib = val & 0xF;
		dst[i] = (char)(nib < 10 ? '0' + nib : 'a' + nib - 10);
		val >>= 4;
	}
}

static size_t make_entry(char *buf, const char *name, uint32_t mode,
			 const char *data, uint32_t datalen)
{
	uint32_t namesize = (uint32_t)strlen(name) + 1;
	memset(buf, '0', 110);
	memcpy(buf, "070701", 6);
	write_hex8(buf + 14, mode);
	write_hex8(buf + 54, datalen);
	write_hex8(buf + 94, namesize);

	size_t name_off = 110;
	memcpy(buf + name_off, name, namesize);
	size_t data_off = (name_off + namesize + 3) & ~(size_t)3;
	if (datalen > 0 && data)
		memcpy(buf + data_off, data, datalen);
	return (data_off + datalen + 3) & ~(size_t)3;
}

static size_t make_trailer(char *buf)
{
	return make_entry(buf, "TRAILER!!!", 0, NULL, 0);
}

static void test_ramfs_cpio_dirs(void)
{
	/* Build a cpio with: dir "cpiotest/", file "cpiotest/msg.txt" */
	char archive[1024];
	memset(archive, 0, sizeof(archive));
	size_t off = 0;

	off += make_entry(archive + off, "cpiotest", 040755, NULL, 0);
	off += make_entry(archive + off, "cpiotest/msg.txt", 0100644,
			  "cpio works!", 11);
	off += make_trailer(archive + off);

	int ret = cpio_extract_all(archive, off);
	KTEST_EQ(ret, 0, "cpio_extract_all succeeds");

	struct inode *dir = vfs_namei("/cpiotest");
	KTEST_NOT_NULL(dir, "/cpiotest exists");
	if (dir)
		KTEST_TRUE(S_ISDIR(dir->mode), "/cpiotest is dir");

	struct inode *file = vfs_namei("/cpiotest/msg.txt");
	KTEST_NOT_NULL(file, "/cpiotest/msg.txt exists");
	if (file) {
		KTEST_TRUE(S_ISREG(file->mode), "msg.txt is regular");
		KTEST_EQ((long long)file->size, 11, "msg.txt size is 11");

		char buf[32];
		memset(buf, 0, sizeof(buf));
		struct file f = { .f_inode = file, .f_op = file->f_op, .f_pos = 0 };
		long nr = file->f_op->read(&f, buf, sizeof(buf));
		KTEST_EQ(nr, 11, "read returns 11");
		KTEST_TRUE(memcmp(buf, "cpio works!", 11) == 0, "content matches");
	}
}

KTEST_REGISTER(test_ramfs_cpio_dirs, "CPIO: extract dirs + files into ramfs", KTEST_CAT_BOOT)
