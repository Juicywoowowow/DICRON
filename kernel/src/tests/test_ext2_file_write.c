#include "ktest.h"
#include "ext2_test_helper.h"
#include "lib/string.h"

static void test_ext2_file_write(void)
{
	struct ext2_fs *fs = ext2_test_setup();
	KTEST_NOT_NULL(fs, "ext2 test fs mounted");
	if (!fs) return;

	/* Create a file to write into */
	uint32_t ino = ext2_create_file(fs, EXT2_ROOT_INO, "data.bin", 0644);
	KTEST_NE(ino, 0, "create file for write test");
	if (ino == 0) { ext2_test_cleanup(fs); return; }

	struct ext2_inode inode;
	ext2_read_inode(fs, ino, &inode);

	/* 1. Write returns correct byte count */
	const char *msg = "Hello, ext2!\n";
	long ret = ext2_file_write(fs, &inode, ino, msg, 0, 13);
	KTEST_EQ(ret, 13, "write returns 13 bytes");

	/* Re-read inode after write (write updates it) */
	ext2_read_inode(fs, ino, &inode);

	/* 2. Readback matches written data */
	char rbuf[64];
	memset(rbuf, 0, sizeof(rbuf));
	long rret = ext2_file_read(fs, &inode, rbuf, 0, 13);
	KTEST_EQ(rret, 13, "read returns 13 bytes");
	KTEST_TRUE(memcmp(rbuf, msg, 13) == 0,
		   "readback matches written data");

	/* 3. i_size updated after write */
	KTEST_EQ(inode.i_size, 13, "i_size is 13 after write");

	/* 4. i_blocks nonzero after write */
	KTEST_GT(inode.i_blocks, 0, "i_blocks > 0 after write");

	/* 5. Write at non-zero offset */
	const char *patch = "PATCHED";
	ret = ext2_file_write(fs, &inode, ino, patch, 7, 7);
	ext2_read_inode(fs, ino, &inode);
	memset(rbuf, 0, sizeof(rbuf));
	ext2_file_read(fs, &inode, rbuf, 7, 7);
	KTEST_TRUE(memcmp(rbuf, "PATCHED", 7) == 0,
		   "write at offset 7 reads back correctly");

	/* 6. Write spanning two blocks (cross block boundary) */
	/* Create a second file for this test */
	uint32_t ino2 = ext2_create_file(fs, EXT2_ROOT_INO, "big.bin", 0644);
	struct ext2_inode in2;
	ext2_read_inode(fs, ino2, &in2);

	/* Write 1500 bytes (crosses 1024-byte block boundary) */
	char bigbuf[1500];
	memset(bigbuf, 'A', 1024);
	memset(bigbuf + 1024, 'B', 476);
	ret = ext2_file_write(fs, &in2, ino2, bigbuf, 0, 1500);
	KTEST_EQ(ret, 1500, "cross-block write returns 1500");

	/* 7. Partial read of multi-block file */
	ext2_read_inode(fs, ino2, &in2);
	char partbuf[476];
	memset(partbuf, 0, sizeof(partbuf));
	rret = ext2_file_read(fs, &in2, partbuf, 1024, 476);
	KTEST_TRUE(rret == 476 && partbuf[0] == 'B' && partbuf[475] == 'B',
		   "partial read of second block correct");

	/* 8. i_block[0] populated after write */
	KTEST_NE(inode.i_block[0], 0, "i_block[0] allocated after write");

	ext2_test_cleanup(fs);
}

KTEST_REGISTER(test_ext2_file_write,
	       "ext2: file data write + readback", KTEST_CAT_BOOT)
