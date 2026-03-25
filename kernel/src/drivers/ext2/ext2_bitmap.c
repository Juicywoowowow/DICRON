#include "ext2.h"
#include <dicron/mem.h>
#include <dicron/log.h>
#include "lib/string.h"

/* ── Block bitmap ── */

uint32_t ext2_alloc_block(struct ext2_fs *fs, uint32_t group_hint)
{
	if (!fs) {
		klog(KLOG_ERR, "ext2_alloc_block: NULL fs\n");
		return 0;
	}
	if (group_hint >= fs->groups_count)
		group_hint = 0;

	if (fs->sb.s_free_blocks_count == 0) {
		klog(KLOG_WARN, "ext2: no free blocks (ENOSPC)\n");
		return 0;
	}

	uint8_t *bitmap = kmalloc(fs->block_size);
	if (!bitmap) {
		klog(KLOG_ERR, "ext2_alloc_block: OOM for bitmap\n");
		return 0;
	}

	/* Try the hinted group first, then wrap around */
	for (uint32_t attempt = 0; attempt < fs->groups_count; attempt++) {
		uint32_t g = (group_hint + attempt) % fs->groups_count;
		struct ext2_group_desc *gd = &fs->group_descs[g];

		if (gd->bg_free_blocks_count == 0)
			continue;

		if (ext2_read_block(fs, gd->bg_block_bitmap, bitmap) != 0) {
			klog(KLOG_ERR, "ext2: failed to read block bitmap "
			     "for group %u\n", g);
			continue;
		}

		uint32_t bits = fs->sb.s_blocks_per_group;
		/* Last group may have fewer blocks */
		if (g == fs->groups_count - 1) {
			uint32_t rem = fs->sb.s_blocks_count %
				       fs->sb.s_blocks_per_group;
			if (rem != 0)
				bits = rem;
		}

		for (uint32_t bit = 0; bit < bits; bit++) {
			uint32_t byte_idx = bit / 8;
			uint8_t  bit_mask = (uint8_t)(1U << (bit % 8));

			if (!(bitmap[byte_idx] & bit_mask)) {
				/* Found free block — mark it used */
				bitmap[byte_idx] |= bit_mask;

				if (ext2_write_block(fs, gd->bg_block_bitmap,
						     bitmap) != 0) {
					klog(KLOG_ERR, "ext2: failed to write "
					     "block bitmap for group %u\n", g);
					kfree(bitmap);
					return 0;
				}

				gd->bg_free_blocks_count--;
				fs->sb.s_free_blocks_count--;

				kfree(bitmap);

				uint32_t block_nr =
					g * fs->sb.s_blocks_per_group +
					bit + fs->sb.s_first_data_block;
				return block_nr;
			}
		}
	}

	kfree(bitmap);
	klog(KLOG_WARN, "ext2: block alloc exhausted all groups\n");
	return 0;
}

void ext2_free_block(struct ext2_fs *fs, uint32_t block_nr)
{
	if (!fs || block_nr == 0) {
		klog(KLOG_ERR, "ext2_free_block: invalid args "
		     "(fs=%p, block=%u)\n", (void *)fs, block_nr);
		return;
	}
	if (block_nr >= fs->sb.s_blocks_count) {
		klog(KLOG_ERR, "ext2_free_block: block %u out of range "
		     "(max %u)\n", block_nr, fs->sb.s_blocks_count);
		return;
	}

	uint32_t adjusted = block_nr - fs->sb.s_first_data_block;
	uint32_t g = adjusted / fs->sb.s_blocks_per_group;
	uint32_t bit = adjusted % fs->sb.s_blocks_per_group;

	if (g >= fs->groups_count) {
		klog(KLOG_ERR, "ext2_free_block: group %u out of range\n", g);
		return;
	}

	struct ext2_group_desc *gd = &fs->group_descs[g];

	uint8_t *bitmap = kmalloc(fs->block_size);
	if (!bitmap) {
		klog(KLOG_ERR, "ext2_free_block: OOM for bitmap\n");
		return;
	}

	if (ext2_read_block(fs, gd->bg_block_bitmap, bitmap) != 0) {
		klog(KLOG_ERR, "ext2: failed to read block bitmap "
		     "for group %u\n", g);
		kfree(bitmap);
		return;
	}

	uint32_t byte_idx = bit / 8;
	uint8_t  bit_mask = (uint8_t)(1U << (bit % 8));

	if (!(bitmap[byte_idx] & bit_mask)) {
		klog(KLOG_WARN, "ext2: double-free of block %u\n", block_nr);
		kfree(bitmap);
		return;
	}

	bitmap[byte_idx] &= (uint8_t)~bit_mask;
	if (ext2_write_block(fs, gd->bg_block_bitmap, bitmap) != 0) {
		klog(KLOG_ERR, "ext2: failed to write block bitmap "
		     "for group %u\n", g);
		kfree(bitmap);
		return;
	}

	gd->bg_free_blocks_count++;
	fs->sb.s_free_blocks_count++;
	kfree(bitmap);
}

/* ── Inode bitmap ── */

uint32_t ext2_alloc_inode(struct ext2_fs *fs, uint32_t group_hint)
{
	if (!fs) {
		klog(KLOG_ERR, "ext2_alloc_inode: NULL fs\n");
		return 0;
	}
	if (group_hint >= fs->groups_count)
		group_hint = 0;

	if (fs->sb.s_free_inodes_count == 0) {
		klog(KLOG_WARN, "ext2: no free inodes (ENOSPC)\n");
		return 0;
	}

	uint8_t *bitmap = kmalloc(fs->block_size);
	if (!bitmap) {
		klog(KLOG_ERR, "ext2_alloc_inode: OOM for bitmap\n");
		return 0;
	}

	for (uint32_t attempt = 0; attempt < fs->groups_count; attempt++) {
		uint32_t g = (group_hint + attempt) % fs->groups_count;
		struct ext2_group_desc *gd = &fs->group_descs[g];

		if (gd->bg_free_inodes_count == 0)
			continue;

		if (ext2_read_block(fs, gd->bg_inode_bitmap, bitmap) != 0) {
			klog(KLOG_ERR, "ext2: failed to read inode bitmap "
			     "for group %u\n", g);
			continue;
		}

		uint32_t bits = fs->sb.s_inodes_per_group;

		for (uint32_t bit = 0; bit < bits; bit++) {
			uint32_t byte_idx = bit / 8;
			uint8_t  bit_mask = (uint8_t)(1U << (bit % 8));

			if (!(bitmap[byte_idx] & bit_mask)) {
				bitmap[byte_idx] |= bit_mask;

				if (ext2_write_block(fs, gd->bg_inode_bitmap,
						     bitmap) != 0) {
					klog(KLOG_ERR, "ext2: failed to write "
					     "inode bitmap for group %u\n", g);
					kfree(bitmap);
					return 0;
				}

				gd->bg_free_inodes_count--;
				fs->sb.s_free_inodes_count--;

				kfree(bitmap);

				/* Inode numbers are 1-based */
				uint32_t ino =
					g * fs->sb.s_inodes_per_group +
					bit + 1;
				return ino;
			}
		}
	}

	kfree(bitmap);
	klog(KLOG_WARN, "ext2: inode alloc exhausted all groups\n");
	return 0;
}

void ext2_free_inode(struct ext2_fs *fs, uint32_t ino)
{
	if (!fs || ino == 0) {
		klog(KLOG_ERR, "ext2_free_inode: invalid args "
		     "(fs=%p, ino=%u)\n", (void *)fs, ino);
		return;
	}
	if (ino > fs->sb.s_inodes_count) {
		klog(KLOG_ERR, "ext2_free_inode: inode %u out of range "
		     "(max %u)\n", ino, fs->sb.s_inodes_count);
		return;
	}

	uint32_t g = (ino - 1) / fs->sb.s_inodes_per_group;
	uint32_t bit = (ino - 1) % fs->sb.s_inodes_per_group;

	if (g >= fs->groups_count) {
		klog(KLOG_ERR, "ext2_free_inode: group %u out of range\n", g);
		return;
	}

	struct ext2_group_desc *gd = &fs->group_descs[g];

	uint8_t *bitmap = kmalloc(fs->block_size);
	if (!bitmap) {
		klog(KLOG_ERR, "ext2_free_inode: OOM for bitmap\n");
		return;
	}

	if (ext2_read_block(fs, gd->bg_inode_bitmap, bitmap) != 0) {
		klog(KLOG_ERR, "ext2: failed to read inode bitmap "
		     "for group %u\n", g);
		kfree(bitmap);
		return;
	}

	uint32_t byte_idx = bit / 8;
	uint8_t  bit_mask = (uint8_t)(1U << (bit % 8));

	if (!(bitmap[byte_idx] & bit_mask)) {
		klog(KLOG_WARN, "ext2: double-free of inode %u\n", ino);
		kfree(bitmap);
		return;
	}

	bitmap[byte_idx] &= (uint8_t)~bit_mask;
	if (ext2_write_block(fs, gd->bg_inode_bitmap, bitmap) != 0) {
		klog(KLOG_ERR, "ext2: failed to write inode bitmap "
		     "for group %u\n", g);
		kfree(bitmap);
		return;
	}

	gd->bg_free_inodes_count++;
	fs->sb.s_free_inodes_count++;
	kfree(bitmap);
}
