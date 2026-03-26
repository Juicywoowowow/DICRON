#include "ext2.h"
#include <dicron/mem.h>
#include <dicron/log.h>
#include "lib/string.h"

/* ── Add a directory entry to a directory ── */

int ext2_dir_add_entry(struct ext2_fs *fs, struct ext2_inode *dir,
		       uint32_t dir_ino, const char *name,
		       uint32_t child_ino, uint8_t file_type)
{
	if (!fs || !dir || !name) {
		klog(KLOG_ERR, "ext2_dir_add_entry: NULL arg\n");
		return -1;
	}

	size_t name_len = strlen(name);
	if (name_len == 0 || name_len > 255) {
		klog(KLOG_ERR, "ext2_dir_add_entry: bad name_len %u\n",
		     (unsigned)name_len);
		return -1;
	}

	/* Size needed for the new entry (8-byte header + name, 4-aligned) */
	uint16_t need = (uint16_t)((8U + name_len + 3U) & ~3U);

	uint8_t *buf = kmalloc(fs->block_size);
	if (!buf) {
		klog(KLOG_ERR, "ext2_dir_add_entry: OOM\n");
		return -1;
	}

	uint32_t dir_blocks = dir->i_size / fs->block_size;

	/* Scan existing directory blocks for space */
	for (uint32_t b = 0; b < dir_blocks; b++) {
		uint32_t block_nr = ext2_inode_block_nr(fs, dir, b);
		if (block_nr == 0)
			continue;
		if (ext2_read_block(fs, block_nr, buf) != 0)
			continue;

		uint32_t offset = 0;
		while (offset < fs->block_size) {
			struct ext2_dir_entry *de =
				(struct ext2_dir_entry *)(buf + offset);
			if (de->rec_len == 0)
				break;

			/* Actual size this entry uses */
			uint16_t actual = (uint16_t)((8U + de->name_len + 3U) & ~3U);
			/* If inode == 0 the entry is deleted — full rec_len available */
			uint16_t avail;
			if (de->inode == 0) {
				avail = de->rec_len;
			} else {
				avail = de->rec_len - actual;
			}

			if (avail >= need) {
				if (de->inode != 0) {
					/* Split: shrink current entry */
					uint16_t old_rec = de->rec_len;
					de->rec_len = actual;

					/* New entry starts after current */
					struct ext2_dir_entry *ne =
						(struct ext2_dir_entry *)
						(buf + offset + actual);
					ne->inode = child_ino;
					ne->rec_len = old_rec - actual;
					ne->name_len = (uint8_t)name_len;
					ne->file_type = file_type;
					memcpy(ne->name, name, name_len);
				} else {
					/* Reuse deleted entry */
					de->inode = child_ino;
					de->name_len = (uint8_t)name_len;
					de->file_type = file_type;
					memcpy(de->name, name, name_len);
				}

				if (ext2_write_block(fs, block_nr, buf) != 0) {
					klog(KLOG_ERR, "ext2_dir_add_entry: "
					     "failed to write dir block\n");
					kfree(buf);
					return -1;
				}
				kfree(buf);
				return 0;
			}

			offset += de->rec_len;
		}
	}

	/* No space in existing blocks — allocate a new one */
	uint32_t new_block = ext2_alloc_block(fs, 0);
	if (new_block == 0) {
		klog(KLOG_ERR, "ext2_dir_add_entry: no free blocks for "
		     "dir expansion\n");
		kfree(buf);
		return -1;
	}

	memset(buf, 0, fs->block_size);
	struct ext2_dir_entry *de = (struct ext2_dir_entry *)buf;
	de->inode = child_ino;
	de->rec_len = (uint16_t)fs->block_size;
	de->name_len = (uint8_t)name_len;
	de->file_type = file_type;
	memcpy(de->name, name, name_len);

	if (ext2_write_block(fs, new_block, buf) != 0) {
		klog(KLOG_ERR, "ext2_dir_add_entry: failed to write "
		     "new dir block\n");
		ext2_free_block(fs, new_block);
		kfree(buf);
		return -1;
	}

	/* Assign the new block to the next file_block slot */
	uint32_t slot = dir_blocks;
	if (slot < EXT2_NDIR_BLOCKS) {
		dir->i_block[slot] = new_block;
	} else {
		klog(KLOG_ERR, "ext2_dir_add_entry: indirect dir blocks "
		     "not yet supported\n");
		ext2_free_block(fs, new_block);
		kfree(buf);
		return -1;
	}

	dir->i_size += fs->block_size;
	dir->i_blocks += fs->block_size / 512;

	/* Write updated parent inode */
	if (ext2_write_inode(fs, dir_ino, dir) != 0) {
		klog(KLOG_ERR, "ext2_dir_add_entry: failed to write "
		     "parent inode %u\n", dir_ino);
		kfree(buf);
		return -1;
	}

	kfree(buf);
	return 0;
}

/* ── Create a regular file ── */

uint32_t ext2_create_file(struct ext2_fs *fs, uint32_t parent_ino,
			  const char *name, uint32_t mode)
{
	if (!fs || !name) {
		klog(KLOG_ERR, "ext2_create_file: NULL arg\n");
		return 0;
	}

	/* Allocate inode */
	uint32_t ino = ext2_alloc_inode(fs, 0);
	if (ino == 0) {
		klog(KLOG_ERR, "ext2_create_file: failed to alloc inode\n");
		return 0;
	}

	/* Initialize the inode */
	struct ext2_inode inode;
	memset(&inode, 0, sizeof(inode));
	inode.i_mode = (uint16_t)(EXT2_S_IFREG | (mode & 0777));
	inode.i_links_count = 1;
	inode.i_size = 0;
	inode.i_blocks = 0;

	if (ext2_write_inode(fs, ino, &inode) != 0) {
		klog(KLOG_ERR, "ext2_create_file: failed to write new "
		     "inode %u\n", ino);
		ext2_free_inode(fs, ino);
		return 0;
	}

	/* Read parent directory inode */
	struct ext2_inode parent;
	if (ext2_read_inode(fs, parent_ino, &parent) != 0) {
		klog(KLOG_ERR, "ext2_create_file: failed to read parent "
		     "inode %u\n", parent_ino);
		ext2_free_inode(fs, ino);
		return 0;
	}

	/* Add directory entry in parent */
	if (ext2_dir_add_entry(fs, &parent, parent_ino, name,
			       ino, EXT2_FT_REG_FILE) != 0) {
		klog(KLOG_ERR, "ext2_create_file: failed to add dir entry "
		     "'%s' in inode %u\n", name, parent_ino);
		ext2_free_inode(fs, ino);
		return 0;
	}

	klog(KLOG_DEBUG, "ext2: created file '%s' -> inode %u\n", name, ino);
	return ino;
}

/* ── Create a directory ── */

uint32_t ext2_mkdir(struct ext2_fs *fs, uint32_t parent_ino,
		    const char *name, uint32_t mode)
{
	if (!fs || !name) {
		klog(KLOG_ERR, "ext2_mkdir: NULL arg\n");
		return 0;
	}

	/* Allocate inode */
	uint32_t ino = ext2_alloc_inode(fs, 0);
	if (ino == 0) {
		klog(KLOG_ERR, "ext2_mkdir: failed to alloc inode\n");
		return 0;
	}

	/* Allocate one data block for '.' and '..' entries */
	uint32_t data_block = ext2_alloc_block(fs, 0);
	if (data_block == 0) {
		klog(KLOG_ERR, "ext2_mkdir: failed to alloc data block\n");
		ext2_free_inode(fs, ino);
		return 0;
	}

	/* Write '.' and '..' entries */
	uint8_t *buf = kmalloc(fs->block_size);
	if (!buf) {
		klog(KLOG_ERR, "ext2_mkdir: OOM\n");
		ext2_free_block(fs, data_block);
		ext2_free_inode(fs, ino);
		return 0;
	}

	memset(buf, 0, fs->block_size);

	/* '.' entry: points to self */
	struct ext2_dir_entry *dot = (struct ext2_dir_entry *)buf;
	dot->inode = ino;
	dot->rec_len = 12;  /* 8 + 1 name byte, 4-aligned = 12 */
	dot->name_len = 1;
	dot->file_type = EXT2_FT_DIR;
	dot->name[0] = '.';

	/* '..' entry: points to parent, takes remaining space */
	struct ext2_dir_entry *dotdot =
		(struct ext2_dir_entry *)(buf + 12);
	dotdot->inode = parent_ino;
	dotdot->rec_len = (uint16_t)(fs->block_size - 12);
	dotdot->name_len = 2;
	dotdot->file_type = EXT2_FT_DIR;
	dotdot->name[0] = '.';
	dotdot->name[1] = '.';

	if (ext2_write_block(fs, data_block, buf) != 0) {
		klog(KLOG_ERR, "ext2_mkdir: failed to write dir block\n");
		kfree(buf);
		ext2_free_block(fs, data_block);
		ext2_free_inode(fs, ino);
		return 0;
	}
	kfree(buf);

	/* Initialize the directory inode */
	struct ext2_inode inode;
	memset(&inode, 0, sizeof(inode));
	inode.i_mode = (uint16_t)(EXT2_S_IFDIR | (mode & 0777));
	inode.i_links_count = 2;  /* '.' + parent's entry */
	inode.i_size = fs->block_size;
	inode.i_blocks = fs->block_size / 512;
	inode.i_block[0] = data_block;

	if (ext2_write_inode(fs, ino, &inode) != 0) {
		klog(KLOG_ERR, "ext2_mkdir: failed to write inode %u\n", ino);
		ext2_free_block(fs, data_block);
		ext2_free_inode(fs, ino);
		return 0;
	}

	/* Read parent, add dir entry, increment parent links */
	struct ext2_inode parent;
	if (ext2_read_inode(fs, parent_ino, &parent) != 0) {
		klog(KLOG_ERR, "ext2_mkdir: failed to read parent "
		     "inode %u\n", parent_ino);
		ext2_free_block(fs, data_block);
		ext2_free_inode(fs, ino);
		return 0;
	}

	if (ext2_dir_add_entry(fs, &parent, parent_ino, name,
			       ino, EXT2_FT_DIR) != 0) {
		klog(KLOG_ERR, "ext2_mkdir: failed to add dir entry '%s' "
		     "in inode %u\n", name, parent_ino);
		ext2_free_block(fs, data_block);
		ext2_free_inode(fs, ino);
		return 0;
	}

	/* Increment parent link count for '..' reference */
	parent.i_links_count++;
	if (ext2_write_inode(fs, parent_ino, &parent) != 0) {
		klog(KLOG_WARN, "ext2_mkdir: failed to update parent "
		     "link count\n");
	}

	klog(KLOG_DEBUG, "ext2: created dir '%s' -> inode %u\n", name, ino);
	return ino;
}

/* ── Write data to a file ── */

long ext2_file_write(struct ext2_fs *fs, struct ext2_inode *inode,
		     uint32_t ino, const void *data,
		     size_t offset, size_t count)
{
	if (!fs || !inode || !data) {
		klog(KLOG_ERR, "ext2_file_write: NULL arg\n");
		return -1;
	}
	if (count == 0)
		return 0;

	const uint8_t *src = data;
	size_t remaining = count;
	uint8_t *block_buf = kmalloc(fs->block_size);
	if (!block_buf) {
		klog(KLOG_ERR, "ext2_file_write: OOM\n");
		return -1;
	}

	while (remaining > 0) {
		uint32_t file_block = (uint32_t)(offset / fs->block_size);
		uint32_t block_offset = (uint32_t)(offset % fs->block_size);

		size_t chunk = fs->block_size - block_offset;
		if (chunk > remaining)
			chunk = remaining;

		/* Get or allocate the block */
		uint32_t block_nr = ext2_inode_block_nr(fs, inode, file_block);
		if (block_nr == 0) {
			/* Need to allocate a new block */
			block_nr = ext2_alloc_block(fs, 0);
			if (block_nr == 0) {
				klog(KLOG_ERR, "ext2_file_write: "
				     "block alloc failed at file_block %u\n",
				     file_block);
				kfree(block_buf);
				return -1;
			}

			/* Assign to appropriate slot */
			if (file_block < EXT2_NDIR_BLOCKS) {
				inode->i_block[file_block] = block_nr;
			} else if (file_block - EXT2_NDIR_BLOCKS <
				   fs->addrs_per_block) {
				/* Single indirect block */
				uint32_t idx = file_block - EXT2_NDIR_BLOCKS;
				uint32_t ind_blk = inode->i_block[EXT2_IND_BLOCK];
				uint32_t *ind = kmalloc(fs->block_size);
				if (!ind) {
					ext2_free_block(fs, block_nr);
					kfree(block_buf);
					return -1;
				}
				if (ind_blk == 0) {
					ind_blk = ext2_alloc_block(fs, 0);
					if (ind_blk == 0) {
						kfree(ind);
						ext2_free_block(fs, block_nr);
						kfree(block_buf);
						return -1;
					}
					memset(ind, 0, fs->block_size);
					inode->i_block[EXT2_IND_BLOCK] = ind_blk;
				} else {
					if (ext2_read_block(fs, ind_blk, ind) != 0) {
						kfree(ind);
						ext2_free_block(fs, block_nr);
						kfree(block_buf);
						return -1;
					}
				}
				ind[idx] = block_nr;
				ext2_write_block(fs, ind_blk, ind);
				kfree(ind);
			} else {
				klog(KLOG_ERR, "ext2_file_write: double/triple "
				     "indirect write not yet supported\n");
				ext2_free_block(fs, block_nr);
				kfree(block_buf);
				return -1;
			}

			/* Zero the new block first */
			memset(block_buf, 0, fs->block_size);
		} else {
			/* Read existing block if partial write */
			if (block_offset != 0 || chunk < fs->block_size) {
				if (ext2_read_block(fs, block_nr,
						    block_buf) != 0) {
					klog(KLOG_ERR, "ext2_file_write: "
					     "failed to read block %u\n",
					     block_nr);
					kfree(block_buf);
					return -1;
				}
			}
		}

		/* Copy data into block buffer */
		memcpy(block_buf + block_offset, src, chunk);

		/* Write block to disk */
		if (ext2_write_block(fs, block_nr, block_buf) != 0) {
			klog(KLOG_ERR, "ext2_file_write: failed to write "
			     "block %u\n", block_nr);
			kfree(block_buf);
			return -1;
		}

		src += chunk;
		offset += chunk;
		remaining -= chunk;
	}

	/* Update inode metadata */
	if (offset > inode->i_size)
		inode->i_size = (uint32_t)offset;

	/* Recount blocks (512-byte sectors) */
	uint32_t file_blocks_count = (inode->i_size + fs->block_size - 1) /
				     fs->block_size;
	inode->i_blocks = file_blocks_count * (fs->block_size / 512);

	if (ext2_write_inode(fs, ino, inode) != 0) {
		klog(KLOG_ERR, "ext2_file_write: failed to write-back "
		     "inode %u\n", ino);
		kfree(block_buf);
		return -1;
	}

	kfree(block_buf);
	return (long)count;
}

/* ── Unlink a file from a directory ── */

int ext2_unlink(struct ext2_fs *fs, uint32_t parent_ino, const char *name)
{
	if (!fs || !name) {
		klog(KLOG_ERR, "ext2_unlink: NULL arg\n");
		return -1;
	}

	struct ext2_inode parent;
	if (ext2_read_inode(fs, parent_ino, &parent) != 0) {
		klog(KLOG_ERR, "ext2_unlink: failed to read parent "
		     "inode %u\n", parent_ino);
		return -1;
	}

	size_t name_len = strlen(name);
	uint32_t dir_blocks = parent.i_size / fs->block_size;

	uint8_t *buf = kmalloc(fs->block_size);
	if (!buf) {
		klog(KLOG_ERR, "ext2_unlink: OOM\n");
		return -1;
	}

	for (uint32_t b = 0; b < dir_blocks; b++) {
		uint32_t block_nr = ext2_inode_block_nr(fs, &parent, b);
		if (block_nr == 0)
			continue;
		if (ext2_read_block(fs, block_nr, buf) != 0)
			continue;

		uint32_t offset = 0;
		struct ext2_dir_entry *prev = NULL;

		while (offset < fs->block_size) {
			struct ext2_dir_entry *de =
				(struct ext2_dir_entry *)(buf + offset);
			if (de->rec_len == 0)
				break;

			if (de->inode != 0 &&
			    de->name_len == (uint8_t)name_len &&
			    memcmp(de->name, name, name_len) == 0) {
				/* Found it */
				uint32_t child_ino = de->inode;

				if (prev) {
					/* Merge rec_len with previous entry */
					prev->rec_len += de->rec_len;
				} else {
					/* First entry — just zero the inode */
					de->inode = 0;
				}

				if (ext2_write_block(fs, block_nr, buf) != 0) {
					klog(KLOG_ERR, "ext2_unlink: failed "
					     "to write dir block\n");
					kfree(buf);
					return -1;
				}

				/* Decrement link count and free if zero */
				struct ext2_inode child;
				if (ext2_read_inode(fs, child_ino,
						    &child) != 0) {
					klog(KLOG_ERR, "ext2_unlink: failed "
					     "to read child inode %u\n",
					     child_ino);
					kfree(buf);
					return -1;
				}

				if (child.i_links_count > 0)
					child.i_links_count--;

				if (child.i_links_count == 0) {
					/* Free all direct data blocks */
					for (uint32_t i = 0;
					     i < EXT2_NDIR_BLOCKS; i++) {
						if (child.i_block[i] != 0) {
							ext2_free_block(
								fs,
								child.i_block[i]);
							child.i_block[i] = 0;
						}
					}
					/* Free single indirect block and its entries */
					if (child.i_block[EXT2_IND_BLOCK] != 0) {
						uint32_t *ind = kmalloc(fs->block_size);
						if (ind) {
							if (ext2_read_block(fs,
							    child.i_block[EXT2_IND_BLOCK],
							    ind) == 0) {
								for (uint32_t j = 0;
								     j < fs->addrs_per_block;
								     j++) {
									if (ind[j] != 0)
										ext2_free_block(fs, ind[j]);
								}
							}
							kfree(ind);
						}
						ext2_free_block(fs,
						    child.i_block[EXT2_IND_BLOCK]);
						child.i_block[EXT2_IND_BLOCK] = 0;
					}
					/* Free double indirect block and its entries */
					if (child.i_block[EXT2_DIND_BLOCK] != 0) {
						uint32_t *dind = kmalloc(fs->block_size);
						if (dind) {
							if (ext2_read_block(fs,
							    child.i_block[EXT2_DIND_BLOCK],
							    dind) == 0) {
								for (uint32_t j = 0;
								     j < fs->addrs_per_block;
								     j++) {
									if (dind[j] == 0)
										continue;
									uint32_t *ind = kmalloc(fs->block_size);
									if (ind) {
										if (ext2_read_block(fs, dind[j], ind) == 0) {
											for (uint32_t k = 0;
											     k < fs->addrs_per_block;
											     k++) {
												if (ind[k] != 0)
													ext2_free_block(fs, ind[k]);
											}
										}
										kfree(ind);
									}
									ext2_free_block(fs, dind[j]);
								}
							}
							kfree(dind);
						}
						ext2_free_block(fs,
						    child.i_block[EXT2_DIND_BLOCK]);
						child.i_block[EXT2_DIND_BLOCK] = 0;
					}
					child.i_size = 0;
					child.i_blocks = 0;

					ext2_write_inode(fs, child_ino, &child);
					ext2_free_inode(fs, child_ino);

					klog(KLOG_DEBUG, "ext2: unlinked "
					     "'%s' (inode %u freed)\n",
					     name, child_ino);
				} else {
					ext2_write_inode(fs, child_ino, &child);
					klog(KLOG_DEBUG, "ext2: unlinked "
					     "'%s' (inode %u links=%u)\n",
					     name, child_ino,
					     child.i_links_count);
				}

				kfree(buf);
				return 0;
			}

			prev = de;
			offset += de->rec_len;
		}
	}

	kfree(buf);
	klog(KLOG_WARN, "ext2_unlink: '%s' not found in inode %u\n",
	     name, parent_ino);
	return -1;
}
