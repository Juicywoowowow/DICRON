#include "ktest.h"
#include "ext2_test_helper.h"
#include "lib/string.h"

static void test_ext2_unlink(void)
{
	struct ext2_fs *fs = ext2_test_setup();
	KTEST_NOT_NULL(fs, "ext2 test fs mounted");
	if (!fs) return;

	/* Create two files so we can test sibling survival */
	uint32_t ino_a = ext2_create_file(fs, EXT2_ROOT_INO, "remove_me", 0644);
	uint32_t ino_b = ext2_create_file(fs, EXT2_ROOT_INO, "keep_me", 0644);

	/* Write some data to remove_me so it has allocated blocks */
	struct ext2_inode inode_a;
	ext2_read_inode(fs, ino_a, &inode_a);
	ext2_file_write(fs, &inode_a, ino_a, "payload", 0, 7);

	/* Snapshot free counts before unlink */
	uint32_t ino_free_before = fs->group_descs[0].bg_free_inodes_count;
	uint32_t blk_free_before = fs->group_descs[0].bg_free_blocks_count;
	uint32_t sb_ino_before   = fs->sb.s_free_inodes_count;

	/* 1. Unlink returns success */
	int ret = ext2_unlink(fs, EXT2_ROOT_INO, "remove_me");
	KTEST_EQ(ret, 0, "unlink returns 0");

	/* 2. File no longer found by lookup */
	struct ext2_inode root;
	ext2_read_inode(fs, EXT2_ROOT_INO, &root);
	uint32_t found = ext2_dir_lookup(fs, &root, "remove_me");
	KTEST_EQ(found, 0, "unlinked file not found by lookup");

	/* 3. Inode link count reaches zero */
	struct ext2_inode dead;
	ext2_read_inode(fs, ino_a, &dead);
	KTEST_EQ(dead.i_links_count, 0, "inode links_count is 0");

	/* 4. bg_free_inodes_count increased by 1 */
	KTEST_EQ(fs->group_descs[0].bg_free_inodes_count,
		 (long long)(ino_free_before + 1),
		 "bg_free_inodes_count increased");

	/* 5. bg_free_blocks_count increased (data blocks freed) */
	KTEST_GT(fs->group_descs[0].bg_free_blocks_count,
		 (long long)blk_free_before,
		 "bg_free_blocks_count increased");

	/* 6. Unlink nonexistent file fails */
	ret = ext2_unlink(fs, EXT2_ROOT_INO, "no_such_file");
	KTEST_NE(ret, 0, "unlink nonexistent returns error");

	/* 7. Sibling file survives */
	ext2_read_inode(fs, EXT2_ROOT_INO, &root);
	uint32_t other = ext2_dir_lookup(fs, &root, "keep_me");
	KTEST_EQ(other, ino_b, "sibling file survives unlink");

	/* 8. s_free_inodes_count updated globally */
	KTEST_EQ(fs->sb.s_free_inodes_count,
		 (long long)(sb_ino_before + 1),
		 "s_free_inodes_count updated");

	ext2_test_cleanup(fs);
}

KTEST_REGISTER(test_ext2_unlink,
	       "ext2: file unlink + resource cleanup", KTEST_CAT_BOOT)
