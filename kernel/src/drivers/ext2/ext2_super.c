#include "ext2.h"
#include <dicron/mem.h>
#include <dicron/log.h>
#include "lib/string.h"
#include <generated/autoconf.h>

struct ext2_fs *ext2_mount(struct blkdev *dev)
{
	if (!dev)
		return NULL;

	struct ext2_fs *fs = kzalloc(sizeof(struct ext2_fs));
	if (!fs)
		return NULL;

	fs->dev = dev;

	/*
	 * Superblock lives at byte offset 1024.
	 * With 1024-byte blocks, that's block 1.
	 * Read it into a temporary buffer.
	 */
	uint8_t buf[1024];
	if (dev->read(dev, 1, buf) != 0) {
		klog(KLOG_ERR, "ext2: failed to read superblock\n");
		kfree(fs);
		return NULL;
	}

	memcpy(&fs->sb, buf, sizeof(struct ext2_superblock));

	if (fs->sb.s_magic != EXT2_MAGIC) {
		klog(KLOG_ERR, "ext2: bad magic 0x%x (expected 0xEF53)\n",
		     fs->sb.s_magic);
		kfree(fs);
		return NULL;
	}

	fs->block_size = 1024U << fs->sb.s_log_block_size;
	fs->inode_size = (fs->sb.s_rev_level >= 1) ? fs->sb.s_inode_size : 128;
	fs->inodes_per_block = fs->block_size / fs->inode_size;
	fs->addrs_per_block = fs->block_size / 4;

	/* Compute number of block groups */
	fs->groups_count =
		(fs->sb.s_blocks_count + fs->sb.s_blocks_per_group - 1) /
		fs->sb.s_blocks_per_group;

	/* Read block group descriptor table.
	 * It starts at the block immediately after the superblock block.
	 * For 1024-byte blocks, superblock is at block 1, so BGD is at block 2.
	 * For larger blocks, superblock fits in block 0 (offset 1024), BGD at block 1. */
	uint32_t bgd_block = (fs->block_size == 1024) ? 2 : 1;
	size_t bgd_size = fs->groups_count * sizeof(struct ext2_group_desc);
	uint32_t bgd_blocks = (uint32_t)((bgd_size + fs->block_size - 1) / fs->block_size);

	fs->group_descs = kmalloc(bgd_blocks * fs->block_size);
	if (!fs->group_descs) {
		kfree(fs);
		return NULL;
	}

	uint8_t *bgd_ptr = (uint8_t *)fs->group_descs;
	for (uint32_t i = 0; i < bgd_blocks; i++) {
		if (ext2_read_block(fs, bgd_block + i, bgd_ptr + i * fs->block_size) != 0) {
			klog(KLOG_ERR, "ext2: failed to read BGD block %u\n",
			     bgd_block + i);
			kfree(fs->group_descs);
			kfree(fs);
			return NULL;
		}
	}

	klog(KLOG_DEBUG, "ext2: mounted — %u blocks, %u inodes, "
	     "block_size=%u, %u groups\n",
	     fs->sb.s_blocks_count, fs->sb.s_inodes_count,
	     fs->block_size, fs->groups_count);

#ifdef CONFIG_EXT2_CHECK
	int check_errors = ext2_check_superblock(fs);
	if (check_errors < 0) {
		klog(KLOG_ERR, "ext2: superblock check failed — "
		     "refusing to mount\n");
		kfree(fs->group_descs);
		kfree(fs);
		return NULL;
	}
#endif

	return fs;
}

void ext2_unmount(struct ext2_fs *fs)
{
	if (!fs) return;
#ifdef CONFIG_EXT2_SYNC
	ext2_sync_unmount(fs);
#endif
	if (fs->group_descs)
		kfree(fs->group_descs);
	kfree(fs);
}
