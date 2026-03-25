#ifndef _DICRON_BLKDEV_H
#define _DICRON_BLKDEV_H

#include <stdint.h>
#include <stddef.h>

struct blkdev {
	void *private;
	size_t block_size;
	uint64_t total_blocks;
	int (*read)(struct blkdev *dev, uint64_t block, void *buf);
	int (*write)(struct blkdev *dev, uint64_t block, const void *buf);
};

/* Ramdisk: memory-backed block device */
struct blkdev *ramdisk_create(void *data, size_t size, size_t block_size);

#endif
