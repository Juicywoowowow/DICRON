#include "ext2.h"
#include <dicron/mem.h>
#include <dicron/log.h>
#include "lib/string.h"

/* ── Superblock validation ── */

int ext2_check_superblock(struct ext2_fs *fs)
{
	if (!fs)
		return -1;

	struct ext2_superblock *sb = &fs->sb;
	int errors = 0;

	if (sb->s_magic != EXT2_MAGIC) {
		klog(KLOG_ERR, "ext2_check: bad magic 0x%x\n", sb->s_magic);
		return -1;
	}

	/* Block size must be 1024, 2048, or 4096 */
	uint32_t bs = 1024U << sb->s_log_block_size;
	if (bs != 1024 && bs != 2048 && bs != 4096) {
		klog(KLOG_ERR, "ext2_check: invalid block size %u\n", bs);
		return -1;
	}

	if (sb->s_blocks_count == 0) {
		klog(KLOG_ERR, "ext2_check: s_blocks_count is 0\n");
		return -1;
	}
	if (sb->s_inodes_count == 0) {
		klog(KLOG_ERR, "ext2_check: s_inodes_count is 0\n");
		return -1;
	}
	if (sb->s_blocks_per_group == 0) {
		klog(KLOG_ERR, "ext2_check: s_blocks_per_group is 0\n");
		return -1;
	}
	if (sb->s_inodes_per_group == 0) {
		klog(KLOG_ERR, "ext2_check: s_inodes_per_group is 0\n");
		return -1;
	}

	/* Verify block groups vs inode groups match */
	uint32_t expected_groups =
		(sb->s_blocks_count + sb->s_blocks_per_group - 1) /
		sb->s_blocks_per_group;
	uint32_t inode_groups =
		(sb->s_inodes_count + sb->s_inodes_per_group - 1) /
		sb->s_inodes_per_group;
	if (expected_groups != inode_groups) {
		klog(KLOG_WARN, "ext2_check: block groups=%u but inode "
		     "groups=%u (mismatch)\n",
		     expected_groups, inode_groups);
		errors++;
	}

	/* Free counts shouldn't exceed totals */
	if (sb->s_free_blocks_count > sb->s_blocks_count) {
		klog(KLOG_WARN, "ext2_check: free_blocks %u > total %u\n",
		     sb->s_free_blocks_count, sb->s_blocks_count);
		errors++;
	}
	if (sb->s_free_inodes_count > sb->s_inodes_count) {
		klog(KLOG_WARN, "ext2_check: free_inodes %u > total %u\n",
		     sb->s_free_inodes_count, sb->s_inodes_count);
		errors++;
	}

	/* Mount state */
	if (sb->s_state != EXT2_VALID_FS) {
		klog(KLOG_WARN, "ext2_check: filesystem not cleanly "
		     "unmounted (state=%u)\n", sb->s_state);
	}

	/* Incompatible features we don't support */
	if (sb->s_feature_incompat != 0) {
		klog(KLOG_WARN, "ext2_check: incompatible features "
		     "0x%x set\n", sb->s_feature_incompat);
	}

	if (errors > 0)
		klog(KLOG_WARN, "ext2_check: superblock has %d "
		     "warning(s)\n", errors);

	return errors;
}

/* ── Inode validation ── */

int ext2_check_inode(struct ext2_fs *fs, uint32_t ino,
		     struct ext2_inode *inode)
{
	if (!fs || !inode)
		return -1;

	int errors = 0;

	/* Validate mode has a recognized type */
	uint16_t type = inode->i_mode & EXT2_S_IFMT;
	if (type != EXT2_S_IFREG && type != EXT2_S_IFDIR &&
	    type != EXT2_S_IFLNK && type != 0) {
		klog(KLOG_WARN, "ext2_check: inode %u has unknown "
		     "type 0x%x\n", ino, type);
		errors++;
	}

	/* Block pointers must be within filesystem */
	for (int i = 0; i < EXT2_N_BLOCKS; i++) {
		if (inode->i_block[i] != 0 &&
		    inode->i_block[i] >= fs->sb.s_blocks_count) {
			klog(KLOG_ERR, "ext2_check: inode %u block[%d]=%u "
			     "out of range (max %u)\n",
			     ino, i, inode->i_block[i],
			     fs->sb.s_blocks_count);
			errors++;
		}
	}

	/* Directories must have nonzero size */
	if (type == EXT2_S_IFDIR && inode->i_size == 0) {
		klog(KLOG_WARN, "ext2_check: dir inode %u has "
		     "zero size\n", ino);
		errors++;
	}

	/* Dead inode without deletion timestamp */
	if (inode->i_links_count == 0 && inode->i_dtime == 0) {
		klog(KLOG_WARN, "ext2_check: inode %u has 0 links "
		     "but no dtime\n", ino);
		errors++;
	}

	return errors;
}

/* ── Directory validation ── */

int ext2_check_dir(struct ext2_fs *fs, struct ext2_inode *dir,
		   uint32_t ino)
{
	if (!fs || !dir)
		return -1;

	if ((dir->i_mode & EXT2_S_IFMT) != EXT2_S_IFDIR)
		return -1;

	int errors = 0;

	uint8_t *buf = kmalloc(fs->block_size);
	if (!buf)
		return -1;

	uint32_t dir_blocks = dir->i_size / fs->block_size;
	int found_dot = 0, found_dotdot = 0;

	for (uint32_t b = 0; b < dir_blocks; b++) {
		uint32_t block_nr = ext2_inode_block_nr(fs, dir, b);
		if (block_nr == 0)
			continue;
		if (block_nr >= fs->sb.s_blocks_count) {
			klog(KLOG_ERR, "ext2_check: dir inode %u "
			     "block[%u]=%u out of range\n",
			     ino, b, block_nr);
			errors++;
			continue;
		}
		if (ext2_read_block(fs, block_nr, buf) != 0) {
			errors++;
			continue;
		}

		uint32_t offset = 0;
		while (offset < fs->block_size) {
			struct ext2_dir_entry *de =
				(struct ext2_dir_entry *)(buf + offset);

			if (de->rec_len == 0)
				break;

			/* rec_len must not overflow the block */
			if (offset + de->rec_len > fs->block_size) {
				klog(KLOG_ERR, "ext2_check: dir inode %u "
				     "rec_len overflow at offset %u\n",
				     ino, offset);
				errors++;
				break;
			}

			if (de->inode != 0) {
				/* Inode number must be valid */
				if (de->inode > fs->sb.s_inodes_count) {
					klog(KLOG_ERR, "ext2_check: dir "
					     "inode %u entry points to "
					     "invalid inode %u\n",
					     ino, de->inode);
					errors++;
				}

				/* Check '.' */
				if (de->name_len == 1 &&
				    de->name[0] == '.') {
					found_dot = 1;
					if (de->inode != ino) {
						klog(KLOG_WARN,
						     "ext2_check: dir %u "
						     "'.' points to %u "
						     "(expected %u)\n",
						     ino, de->inode, ino);
						errors++;
					}
				}
				/* Check '..' */
				if (de->name_len == 2 &&
				    de->name[0] == '.' &&
				    de->name[1] == '.') {
					found_dotdot = 1;
				}
			}

			offset += de->rec_len;
		}
	}

	if (!found_dot) {
		klog(KLOG_WARN, "ext2_check: dir %u missing '.' entry\n",
		     ino);
		errors++;
	}
	if (!found_dotdot) {
		klog(KLOG_WARN, "ext2_check: dir %u missing '..' entry\n",
		     ino);
		errors++;
	}

	kfree(buf);
	return errors;
}
