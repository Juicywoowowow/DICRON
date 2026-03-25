#include "ext2.h"
#include <dicron/mem.h>
#include "lib/string.h"

long ext2_file_read(struct ext2_fs *fs, struct ext2_inode *inode,
		    void *buf, size_t offset, size_t count)
{
	if (!inode || !buf)
		return -1;

	/* Clamp to file size */
	if (offset >= inode->i_size)
		return 0;
	if (offset + count > inode->i_size)
		count = inode->i_size - offset;
	if (count == 0)
		return 0;

	uint8_t *dst = buf;
	size_t remaining = count;
	uint8_t *block_buf = kmalloc(fs->block_size);
	if (!block_buf)
		return -1;

	while (remaining > 0) {
		uint32_t file_block = (uint32_t)(offset / fs->block_size);
		uint32_t block_offset = (uint32_t)(offset % fs->block_size);
		uint32_t block_nr = ext2_inode_block_nr(fs, inode, file_block);

		size_t chunk = fs->block_size - block_offset;
		if (chunk > remaining)
			chunk = remaining;

		if (block_nr == 0) {
			/* Sparse block — return zeros */
			memset(dst, 0, chunk);
		} else {
			if (ext2_read_block(fs, block_nr, block_buf) != 0) {
				kfree(block_buf);
				return -1;
			}
			memcpy(dst, block_buf + block_offset, chunk);
		}

		dst += chunk;
		offset += chunk;
		remaining -= chunk;
	}

	kfree(block_buf);
	return (long)count;
}
