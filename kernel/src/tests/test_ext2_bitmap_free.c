#include "ktest.h"
#include "ext2_test_helper.h"

static void test_ext2_bitmap_free(void)
{
	struct ext2_fs *fs = ext2_test_setup();
	KTEST_NOT_NULL(fs, "ext2 test fs mounted");
	if (!fs) return;

	/* Snapshot original counts */
	uint32_t orig_blk_free   = fs->group_descs[0].bg_free_blocks_count;
	uint32_t orig_ino_free   = fs->group_descs[0].bg_free_inodes_count;
	uint32_t orig_sb_blk     = fs->sb.s_free_blocks_count;
	uint32_t orig_sb_ino     = fs->sb.s_free_inodes_count;

	/* Allocate then free a block */
	uint32_t block1 = ext2_alloc_block(fs, 0);

	/* 1. Free a previously allocated block succeeds (no crash) */
	ext2_free_block(fs, block1);
	KTEST_EQ(fs->group_descs[0].bg_free_blocks_count,
		 (long long)orig_blk_free,
		 "free block: bg count restored");

	/* 2. bg_free_blocks_count restored after alloc+free */
	/* (already asserted above — this confirms via superblock) */
	KTEST_EQ(fs->sb.s_free_blocks_count, (long long)orig_sb_blk,
		 "free block: sb count restored");

	/* 3. Freed block can be re-allocated (first-fit returns same) */
	uint32_t block2 = ext2_alloc_block(fs, 0);
	KTEST_EQ(block1, block2, "freed block re-allocated same number");
	ext2_free_block(fs, block2);  /* clean up */

	/* Allocate then free an inode */
	uint32_t ino1 = ext2_alloc_inode(fs, 0);

	/* 4. Free a previously allocated inode (no crash) */
	ext2_free_inode(fs, ino1);
	KTEST_EQ(fs->group_descs[0].bg_free_inodes_count,
		 (long long)orig_ino_free,
		 "free inode: bg count restored");

	/* 5. bg_free_inodes_count restored */
	KTEST_EQ(fs->sb.s_free_inodes_count, (long long)orig_sb_ino,
		 "free inode: sb count restored");

	/* 6. Freed inode can be re-allocated */
	uint32_t ino2 = ext2_alloc_inode(fs, 0);
	KTEST_EQ(ino1, ino2, "freed inode re-allocated same number");
	ext2_free_inode(fs, ino2);

	/* 7. s_free_blocks_count consistent after full cycle */
	KTEST_EQ(fs->sb.s_free_blocks_count, (long long)orig_sb_blk,
		 "s_free_blocks_count consistent after cycle");

	/* 8. s_free_inodes_count consistent after full cycle */
	KTEST_EQ(fs->sb.s_free_inodes_count, (long long)orig_sb_ino,
		 "s_free_inodes_count consistent after cycle");

	ext2_test_cleanup(fs);
}

KTEST_REGISTER(test_ext2_bitmap_free,
	       "ext2: bitmap block/inode deallocation", KTEST_CAT_BOOT)
