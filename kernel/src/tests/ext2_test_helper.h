#ifndef _EXT2_TEST_HELPER_H
#define _EXT2_TEST_HELPER_H

#include "drivers/ext2/ext2.h"
#include <dicron/blkdev.h>

/*
 * Build a minimal valid 64 KiB ext2 filesystem on a ramdisk.
 * Returns a mounted ext2_fs, or NULL on failure.
 *
 * Layout (1024-byte blocks, 64 blocks total):
 *   Block 0  : boot block (unused)
 *   Block 1  : superblock
 *   Block 2  : group descriptor table
 *   Block 3  : block bitmap
 *   Block 4  : inode bitmap
 *   Block 5-8: inode table (4 blocks = 32 inodes)
 *   Block 9+ : data blocks (root dir uses block 9)
 */
struct ext2_fs *ext2_test_setup(void);

/*
 * Unmount and release the test filesystem.
 */
void ext2_test_cleanup(struct ext2_fs *fs);

#endif
