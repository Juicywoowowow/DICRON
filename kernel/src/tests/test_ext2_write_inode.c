#include "ktest.h"
#include "ext2_test_helper.h"
#include "lib/string.h"

static void test_ext2_write_inode(void)
{
	struct ext2_fs *fs = ext2_test_setup();
	KTEST_NOT_NULL(fs, "ext2 test fs mounted");
	if (!fs) return;

	/* Allocate a fresh inode to write to */
	uint32_t ino = ext2_alloc_inode(fs, 0);
	KTEST_NE(ino, 0, "alloc test inode");
	if (ino == 0) { ext2_test_cleanup(fs); return; }

	/* Prepare an inode with known values */
	struct ext2_inode wi;
	memset(&wi, 0, sizeof(wi));
	wi.i_mode = 0x81A4;           /* S_IFREG | 0644 */
	wi.i_size = 4096;
	wi.i_links_count = 3;
	wi.i_block[0] = 42;

	/* 1. ext2_write_inode returns success */
	int ret = ext2_write_inode(fs, ino, &wi);
	KTEST_EQ(ret, 0, "write_inode returns 0");

	/* Read it back */
	struct ext2_inode ri;
	memset(&ri, 0, sizeof(ri));
	ret = ext2_read_inode(fs, ino, &ri);
	KTEST_EQ(ret, 0, "read_inode after write succeeds");

	/* 2. i_mode round-trips */
	KTEST_EQ(ri.i_mode, 0x81A4, "i_mode persists");

	/* 3. i_size round-trips */
	KTEST_EQ(ri.i_size, 4096, "i_size persists");

	/* 4. i_links_count round-trips */
	KTEST_EQ(ri.i_links_count, 3, "i_links_count persists");

	/* 5. i_block[0] round-trips */
	KTEST_EQ(ri.i_block[0], 42, "i_block[0] persists");

	/* 6. Write superblock succeeds */
	fs->sb.s_mnt_count = 42;
	ret = ext2_write_superblock(fs);
	KTEST_EQ(ret, 0, "write_superblock returns 0");

	/* 7. Write group descs succeeds */
	ret = ext2_write_group_descs(fs);
	KTEST_EQ(ret, 0, "write_group_descs returns 0");

	/* 8. Superblock field persists after re-mount
	 * Re-read the superblock from disk by re-mounting */
	struct ext2_fs *fs2 = ext2_mount(fs->dev);
	KTEST_NOT_NULL(fs2, "re-mount after sb write");
	if (fs2) {
		KTEST_EQ(fs2->sb.s_mnt_count, 42,
			 "s_mnt_count persists after re-mount");
		/* Don't do full cleanup — we share the same backing device.
		 * Just free the fs2 struct. */
		ext2_unmount(fs2);
	}

	/* Original fs is invalid now (shared device). Just cleanup. */
}

KTEST_REGISTER(test_ext2_write_inode,
	       "ext2: inode & superblock write-back", KTEST_CAT_BOOT)
