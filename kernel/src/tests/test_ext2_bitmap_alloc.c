#include "ktest.h"
#include "ext2_test_helper.h"

static void test_ext2_bitmap_alloc(void)
{
	struct ext2_fs *fs = ext2_test_setup();
	KTEST_NOT_NULL(fs, "ext2 test fs mounted");
	if (!fs) return;

	uint32_t old_blk_free = fs->group_descs[0].bg_free_blocks_count;
	uint32_t old_sb_free  = fs->sb.s_free_blocks_count;

	/* 1. Allocate first free block — should succeed */
	uint32_t block1 = ext2_alloc_block(fs, 0);
	KTEST_NE(block1, 0, "alloc block returns nonzero");

	/* 2. Allocated block within filesystem bounds */
	KTEST_LT(block1, (long long)fs->sb.s_blocks_count,
		 "alloc block within fs range");

	/* 3. bg_free_blocks_count decremented */
	KTEST_EQ(fs->group_descs[0].bg_free_blocks_count,
		 (long long)(old_blk_free - 1),
		 "bg_free_blocks_count decremented");

	/* 4. s_free_blocks_count decremented */
	KTEST_EQ(fs->sb.s_free_blocks_count,
		 (long long)(old_sb_free - 1),
		 "s_free_blocks_count decremented");

	/* 5. Allocate first free inode — should succeed */
	uint32_t old_ifree = fs->group_descs[0].bg_free_inodes_count;
	uint32_t ino = ext2_alloc_inode(fs, 0);
	KTEST_NE(ino, 0, "alloc inode returns nonzero");

	/* 6. Allocated inode within range */
	KTEST_TRUE(ino <= fs->sb.s_inodes_count,
		   "alloc inode within fs range");

	/* 7. bg_free_inodes_count decremented */
	KTEST_EQ(fs->group_descs[0].bg_free_inodes_count,
		 (long long)(old_ifree - 1),
		 "bg_free_inodes_count decremented");

	/* 8. Two consecutive allocs return different blocks */
	uint32_t block2 = ext2_alloc_block(fs, 0);
	KTEST_NE(block1, block2, "two allocs return different blocks");

	ext2_test_cleanup(fs);
}

KTEST_REGISTER(test_ext2_bitmap_alloc,
	       "ext2: bitmap block/inode allocation", KTEST_CAT_BOOT)
