#include "ext2.h"
#include <dicron/mem.h>
#include "lib/string.h"

uint32_t ext2_dir_lookup(struct ext2_fs *fs, struct ext2_inode *dir,
			 const char *name)
{
	if (!dir || !name || !(dir->i_mode & EXT2_S_IFDIR))
		return 0;

	size_t name_len = strlen(name);
	uint32_t dir_blocks = dir->i_size / fs->block_size;
	uint8_t *buf = kmalloc(fs->block_size);
	if (!buf)
		return 0;

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

			if (de->inode != 0 &&
			    de->name_len == (uint8_t)name_len &&
			    memcmp(de->name, name, name_len) == 0) {
				uint32_t ino = de->inode;
				kfree(buf);
				return ino;
			}

			offset += de->rec_len;
		}
	}

	kfree(buf);
	return 0;
}
