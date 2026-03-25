#include "ktest.h"
#include "ext2_test_helper.h"
#include "lib/string.h"

static void test_ext2_mkdir(void)
{
	struct ext2_fs *fs = ext2_test_setup();
	KTEST_NOT_NULL(fs, "ext2 test fs mounted");
	if (!fs) return;

	/* Snapshot original parent link count */
	struct ext2_inode root_before;
	ext2_read_inode(fs, EXT2_ROOT_INO, &root_before);
	uint32_t orig_links = root_before.i_links_count;

	/* 1. mkdir returns valid inode number */
	uint32_t ino = ext2_mkdir(fs, EXT2_ROOT_INO, "subdir", 0755);
	KTEST_NE(ino, 0, "mkdir returns nonzero inode");

	/* Read the created directory inode */
	struct ext2_inode dir_inode;
	int ret = ext2_read_inode(fs, ino, &dir_inode);
	KTEST_EQ(ret, 0, "read created dir inode ok");

	/* 2. Created inode has S_IFDIR type */
	KTEST_EQ(dir_inode.i_mode & EXT2_S_IFMT, EXT2_S_IFDIR,
		 "inode is directory type");

	/* 3. New dir contains '.' pointing to self */
	uint32_t dot_ino = ext2_dir_lookup(fs, &dir_inode, ".");
	KTEST_EQ(dot_ino, ino, "'.' points to self");

	/* 4. New dir contains '..' pointing to parent */
	uint32_t dotdot_ino = ext2_dir_lookup(fs, &dir_inode, "..");
	KTEST_EQ(dotdot_ino, (long long)EXT2_ROOT_INO,
		 "'..' points to parent");

	/* 5. New dir has i_links_count == 2 (self '.' + parent entry) */
	KTEST_EQ(dir_inode.i_links_count, 2, "new dir has 2 links");

	/* 6. Parent link count incremented by 1 */
	struct ext2_inode root_after;
	ext2_read_inode(fs, EXT2_ROOT_INO, &root_after);
	KTEST_EQ(root_after.i_links_count, (long long)(orig_links + 1),
		 "parent link count incremented");

	/* 7. Directory discoverable via parent lookup */
	uint32_t found = ext2_dir_lookup(fs, &root_after, "subdir");
	KTEST_EQ(found, ino, "dir_lookup finds new directory");

	/* 8. Nested mkdir succeeds */
	uint32_t child_ino = ext2_mkdir(fs, ino, "nested", 0755);
	KTEST_NE(child_ino, 0, "nested mkdir returns nonzero inode");

	ext2_test_cleanup(fs);
}

KTEST_REGISTER(test_ext2_mkdir,
	       "ext2: directory creation", KTEST_CAT_BOOT)
