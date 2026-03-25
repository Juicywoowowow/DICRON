#include "ktest.h"
#include "ext2_test_helper.h"
#include "lib/string.h"

static void test_ext2_create_file(void)
{
	struct ext2_fs *fs = ext2_test_setup();
	KTEST_NOT_NULL(fs, "ext2 test fs mounted");
	if (!fs) return;

	uint32_t orig_parent_links;
	{
		struct ext2_inode root;
		ext2_read_inode(fs, EXT2_ROOT_INO, &root);
		orig_parent_links = root.i_links_count;
	}

	/* 1. Create file returns valid inode number */
	uint32_t ino1 = ext2_create_file(fs, EXT2_ROOT_INO, "hello.txt", 0644);
	KTEST_NE(ino1, 0, "create_file returns nonzero inode");

	/* Read the created inode */
	struct ext2_inode inode;
	int ret = ext2_read_inode(fs, ino1, &inode);
	KTEST_EQ(ret, 0, "read created inode");

	/* 2. Created inode has S_IFREG type */
	KTEST_EQ(inode.i_mode & EXT2_S_IFMT, EXT2_S_IFREG,
		 "inode is regular file");

	/* 3. New file starts with zero size */
	KTEST_EQ(inode.i_size, 0, "new file has zero size");

	/* 4. New file has i_links_count == 1 */
	KTEST_EQ(inode.i_links_count, 1, "new file has 1 link");

	/* 5. File discoverable by ext2_dir_lookup */
	struct ext2_inode root;
	ext2_read_inode(fs, EXT2_ROOT_INO, &root);
	uint32_t found = ext2_dir_lookup(fs, &root, "hello.txt");
	KTEST_EQ(found, ino1, "dir_lookup finds created file");

	/* 6. Second file gets unique inode */
	uint32_t ino2 = ext2_create_file(fs, EXT2_ROOT_INO, "world.txt", 0644);
	KTEST_NE(ino1, ino2, "second file gets different inode");

	/* 7. Parent dir link count unchanged by file create */
	ext2_read_inode(fs, EXT2_ROOT_INO, &root);
	KTEST_EQ(root.i_links_count, (long long)orig_parent_links,
		 "parent links unchanged after file create");

	/* 8. Permission bits preserved */
	KTEST_EQ(inode.i_mode & 0777, 0644, "permission bits 0644 preserved");

	ext2_test_cleanup(fs);
}

KTEST_REGISTER(test_ext2_create_file,
	       "ext2: file creation", KTEST_CAT_BOOT)
