#include "ext2.h"
#include <dicron/mem.h>
#include "lib/string.h"

int ext2_read_inode(struct ext2_fs *fs, uint32_t ino, struct ext2_inode *out)
{
	if (ino == 0 || ino > fs->sb.s_inodes_count)
		return -1;

	uint32_t group = (ino - 1) / fs->sb.s_inodes_per_group;
	uint32_t index = (ino - 1) % fs->sb.s_inodes_per_group;

	if (group >= fs->groups_count)
		return -1;

	/* Byte offset of this inode within the inode table */
	uint32_t inode_table_block = fs->group_descs[group].bg_inode_table;
	uint32_t byte_offset = index * fs->inode_size;
	uint32_t block_nr = inode_table_block + byte_offset / fs->block_size;
	uint32_t offset_in_block = byte_offset % fs->block_size;

	uint8_t *buf = kmalloc(fs->block_size);
	if (!buf)
		return -1;

	if (ext2_read_block(fs, block_nr, buf) != 0) {
		kfree(buf);
		return -1;
	}

	memcpy(out, buf + offset_in_block, sizeof(struct ext2_inode));
	kfree(buf);
	return 0;
}

uint32_t ext2_inode_block_nr(struct ext2_fs *fs, struct ext2_inode *inode,
			     uint32_t file_block)
{
	/* Direct blocks (0–11) */
	if (file_block < EXT2_NDIR_BLOCKS)
		return inode->i_block[file_block];

	file_block -= EXT2_NDIR_BLOCKS;

	/* Single indirect */
	if (file_block < fs->addrs_per_block) {
		uint32_t ind_block = inode->i_block[EXT2_IND_BLOCK];
		if (ind_block == 0) return 0;

		uint32_t *ind = kmalloc(fs->block_size);
		if (!ind) return 0;
		if (ext2_read_block(fs, ind_block, ind) != 0) {
			kfree(ind);
			return 0;
		}
		uint32_t result = ind[file_block];
		kfree(ind);
		return result;
	}
	file_block -= fs->addrs_per_block;

	/* Double indirect */
	if (file_block < fs->addrs_per_block * fs->addrs_per_block) {
		uint32_t dind_block = inode->i_block[EXT2_DIND_BLOCK];
		if (dind_block == 0) return 0;

		uint32_t *dind = kmalloc(fs->block_size);
		if (!dind) return 0;
		if (ext2_read_block(fs, dind_block, dind) != 0) {
			kfree(dind);
			return 0;
		}

		uint32_t ind_idx = file_block / fs->addrs_per_block;
		uint32_t ind_off = file_block % fs->addrs_per_block;
		uint32_t ind_block = dind[ind_idx];
		kfree(dind);

		if (ind_block == 0) return 0;

		uint32_t *ind = kmalloc(fs->block_size);
		if (!ind) return 0;
		if (ext2_read_block(fs, ind_block, ind) != 0) {
			kfree(ind);
			return 0;
		}
		uint32_t result = ind[ind_off];
		kfree(ind);
		return result;
	}

	/* Triple indirect — not supported yet */
	return 0;
}
