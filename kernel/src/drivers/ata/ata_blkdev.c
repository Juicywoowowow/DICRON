#include "ata.h"
#include <dicron/blkdev.h>
#include <dicron/mem.h>
#include "lib/string.h"

static int ata_blk_read(struct blkdev *dev, uint64_t block, void *buf)
{
	struct ata_drive *drv = dev->private;
	uint32_t sectors_per_block = (uint32_t)(dev->block_size / ATA_SECTOR_SIZE);
	uint64_t lba = block * sectors_per_block;

	return ata_pio_read(drv, lba, sectors_per_block, buf);
}

static int ata_blk_write(struct blkdev *dev, uint64_t block, const void *buf)
{
	struct ata_drive *drv = dev->private;
	uint32_t sectors_per_block = (uint32_t)(dev->block_size / ATA_SECTOR_SIZE);
	uint64_t lba = block * sectors_per_block;

	return ata_pio_write(drv, lba, sectors_per_block, buf);
}

struct blkdev *ata_blkdev_create(struct ata_drive *drv)
{
	if (!drv || !drv->present)
		return NULL;

	struct blkdev *dev = kzalloc(sizeof(struct blkdev));
	if (!dev)
		return NULL;

	dev->private = drv;
	dev->block_size = ATA_SECTOR_SIZE;
	dev->total_blocks = drv->lba28_sectors;
	dev->read = ata_blk_read;
	dev->write = ata_blk_write;

	return dev;
}
