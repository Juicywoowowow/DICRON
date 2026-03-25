#include "ext2.h"
#include "lib/string.h"

int ext2_read_block(struct ext2_fs *fs, uint32_t block_nr, void *buf)
{
	if (block_nr == 0) {
		memset(buf, 0, fs->block_size);
		return 0;
	}

	/*
	 * If fs block_size == dev block_size, single read.
	 * Otherwise, read multiple device blocks to fill one fs block.
	 */
	uint32_t ratio = (uint32_t)(fs->block_size / fs->dev->block_size);
	uint64_t dev_block = (uint64_t)block_nr * ratio;
	uint8_t *dst = buf;

	for (uint32_t i = 0; i < ratio; i++) {
		if (fs->dev->read(fs->dev, dev_block + i,
				  dst + i * fs->dev->block_size) != 0)
			return -1;
	}

	return 0;
}

int ext2_write_block(struct ext2_fs *fs, uint32_t block_nr, const void *buf)
{
	if (block_nr == 0)
		return -1;

	uint32_t ratio = (uint32_t)(fs->block_size / fs->dev->block_size);
	uint64_t dev_block = (uint64_t)block_nr * ratio;
	const uint8_t *src = buf;

	for (uint32_t i = 0; i < ratio; i++) {
		if (fs->dev->write(fs->dev, dev_block + i,
				   src + i * fs->dev->block_size) != 0)
			return -1;
	}

	return 0;
}
