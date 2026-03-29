#include "sata.h"
#include <dicron/blkdev.h>
#include <dicron/mem.h>
#include <generated/autoconf.h>

#ifdef CONFIG_SATA

static int sata_blk_read(struct blkdev *bd, uint64_t block, void *buf)
{
	struct sata_device *dev = bd->private;
	uint32_t sectors_per_block = (uint32_t)(bd->block_size / SATA_SECTOR_SIZE);
	uint64_t lba = block * sectors_per_block;

	return sata_read(dev, lba, sectors_per_block, buf);
}

static int sata_blk_write(struct blkdev *bd, uint64_t block, const void *buf)
{
	struct sata_device *dev = bd->private;
	uint32_t sectors_per_block = (uint32_t)(bd->block_size / SATA_SECTOR_SIZE);
	uint64_t lba = block * sectors_per_block;

	return sata_write(dev, lba, sectors_per_block, buf);
}

struct blkdev *sata_blkdev_create(struct sata_device *dev)
{
	if (!dev || !dev->present)
		return NULL;

	struct blkdev *bd = kzalloc(sizeof(struct blkdev));
	if (!bd)
		return NULL;

	bd->private = dev;
	bd->block_size = SATA_SECTOR_SIZE;
	bd->total_blocks = dev->total_sectors;
	bd->read = sata_blk_read;
	bd->write = sata_blk_write;

	return bd;
}

#endif /* CONFIG_SATA */
