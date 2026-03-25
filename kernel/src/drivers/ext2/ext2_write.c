#include "ext2.h"
#include <dicron/mem.h>
#include <dicron/log.h>
#include "lib/string.h"

/* ── Write inode back to disk ── */

int ext2_write_inode(struct ext2_fs *fs, uint32_t ino, struct ext2_inode *inode)
{
	if (!fs || !inode) {
		klog(KLOG_ERR, "ext2_write_inode: NULL arg "
		     "(fs=%p, inode=%p)\n", (void *)fs, (void *)inode);
		return -1;
	}
	if (ino == 0 || ino > fs->sb.s_inodes_count) {
		klog(KLOG_ERR, "ext2_write_inode: inode %u out of range "
		     "(max %u)\n", ino, fs->sb.s_inodes_count);
		return -1;
	}

	uint32_t group = (ino - 1) / fs->sb.s_inodes_per_group;
	uint32_t index = (ino - 1) % fs->sb.s_inodes_per_group;

	if (group >= fs->groups_count) {
		klog(KLOG_ERR, "ext2_write_inode: group %u >= groups_count "
		     "%u\n", group, fs->groups_count);
		return -1;
	}

	uint32_t inode_table_block = fs->group_descs[group].bg_inode_table;
	uint32_t byte_offset = index * fs->inode_size;
	uint32_t block_nr = inode_table_block + byte_offset / fs->block_size;
	uint32_t offset_in_block = byte_offset % fs->block_size;

	uint8_t *buf = kmalloc(fs->block_size);
	if (!buf) {
		klog(KLOG_ERR, "ext2_write_inode: OOM\n");
		return -1;
	}

	/* Read the block containing this inode */
	if (ext2_read_block(fs, block_nr, buf) != 0) {
		klog(KLOG_ERR, "ext2_write_inode: failed to read inode table "
		     "block %u\n", block_nr);
		kfree(buf);
		return -1;
	}

	/* Patch our inode into the block */
	memcpy(buf + offset_in_block, inode, sizeof(struct ext2_inode));

	/* Write the block back */
	if (ext2_write_block(fs, block_nr, buf) != 0) {
		klog(KLOG_ERR, "ext2_write_inode: failed to write inode table "
		     "block %u\n", block_nr);
		kfree(buf);
		return -1;
	}

	kfree(buf);
	return 0;
}

/* ── Write superblock back to disk ── */

int ext2_write_superblock(struct ext2_fs *fs)
{
	if (!fs) {
		klog(KLOG_ERR, "ext2_write_superblock: NULL fs\n");
		return -1;
	}

	/*
	 * Superblock lives at byte offset 1024.
	 * With 1024-byte blocks, that's block 1.
	 * Read the whole block, patch the superblock bytes, write back.
	 */
	uint8_t buf[1024];
	if (fs->dev->read(fs->dev, 1, buf) != 0) {
		klog(KLOG_ERR, "ext2_write_superblock: failed to read "
		     "superblock block\n");
		return -1;
	}

	memcpy(buf, &fs->sb, sizeof(struct ext2_superblock));

	if (fs->dev->write(fs->dev, 1, buf) != 0) {
		klog(KLOG_ERR, "ext2_write_superblock: failed to write "
		     "superblock block\n");
		return -1;
	}

	klog(KLOG_DEBUG, "ext2: superblock written\n");
	return 0;
}

/* ── Write group descriptors back to disk ── */

int ext2_write_group_descs(struct ext2_fs *fs)
{
	if (!fs || !fs->group_descs) {
		klog(KLOG_ERR, "ext2_write_group_descs: NULL arg\n");
		return -1;
	}

	/* BGD table starts at the block after the superblock.
	 * For 1024-byte blocks: superblock at block 1, BGD at block 2.
	 * For larger blocks: superblock in block 0, BGD at block 1. */
	uint32_t bgd_block = (fs->block_size == 1024) ? 2 : 1;
	size_t bgd_size = fs->groups_count * sizeof(struct ext2_group_desc);
	uint32_t bgd_blocks =
		(uint32_t)((bgd_size + fs->block_size - 1) / fs->block_size);

	uint8_t *bgd_ptr = (uint8_t *)fs->group_descs;
	for (uint32_t i = 0; i < bgd_blocks; i++) {
		if (ext2_write_block(fs, bgd_block + i,
				     bgd_ptr + i * fs->block_size) != 0) {
			klog(KLOG_ERR, "ext2_write_group_descs: failed to "
			     "write BGD block %u\n", bgd_block + i);
			return -1;
		}
	}

	klog(KLOG_DEBUG, "ext2: group descriptors written (%u blocks)\n",
	     bgd_blocks);
	return 0;
}
