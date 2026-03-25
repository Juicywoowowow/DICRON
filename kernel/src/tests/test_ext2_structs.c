#include "ktest.h"
#include "drivers/ext2/ext2.h"

static void test_ext2_structs(void)
{
	/* Verify on-disk structure sizes match ext2 spec */
	KTEST_EQ((long long)sizeof(struct ext2_group_desc), 32,
		 "ext2_group_desc is 32 bytes");
	KTEST_EQ((long long)sizeof(struct ext2_inode), 128,
		 "ext2_inode is 128 bytes");

	/* ext2_dir_entry is variable but the fixed fields are 8 bytes */
	struct ext2_dir_entry *de = (void *)0;
	(void)de;
	KTEST_EQ((long long)sizeof(struct ext2_dir_entry), 8,
		 "ext2_dir_entry fixed part is 8 bytes");

	/* Verify constants */
	KTEST_EQ(EXT2_MAGIC, 0xEF53, "EXT2_MAGIC");
	KTEST_EQ(EXT2_ROOT_INO, 2, "EXT2_ROOT_INO");
	KTEST_EQ(EXT2_N_BLOCKS, 15, "EXT2_N_BLOCKS");
}

KTEST_REGISTER(test_ext2_structs, "ext2: struct sizes match spec", KTEST_CAT_BOOT)
