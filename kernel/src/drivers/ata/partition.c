#include "ata.h"
#include <dicron/blkdev.h>
#include <dicron/mem.h>
#include <dicron/log.h>
#include "lib/string.h"

#define MBR_SIGNATURE_OFFSET 510
#define MBR_SIGNATURE        0xAA55
#define MBR_PART_TABLE_OFF   446
#define MBR_MAX_PARTS        4

struct part_blkdev_priv {
	struct blkdev *parent;
	uint64_t start_sector;
};

int partition_scan(struct blkdev *disk, struct partition_info *out, int max)
{
	if (!disk || !out || max <= 0)
		return 0;

	uint8_t mbr[512];
	if (disk->read(disk, 0, mbr) != 0) {
		klog(KLOG_ERR, "partition: failed to read MBR\n");
		return 0;
	}

	uint16_t sig = (uint16_t)(mbr[510] | (mbr[511] << 8));
	if (sig != MBR_SIGNATURE) {
		klog(KLOG_WARN, "partition: bad MBR signature 0x%x\n",
		     sig);
		return 0;
	}

	int found = 0;
	for (int i = 0; i < MBR_MAX_PARTS && found < max; i++) {
		struct mbr_entry *e = (struct mbr_entry *)
			(mbr + MBR_PART_TABLE_OFF + i * 16);

		if (e->type == 0 || e->sector_count == 0)
			continue;

		out[found].valid = 1;
		out[found].type = e->type;
		out[found].start_lba = e->lba_start;
		out[found].sector_count = e->sector_count;

		klog(KLOG_INFO, "partition %d: type=0x%x start=%u "
		     "sectors=%u\n", i, e->type, e->lba_start,
		     e->sector_count);
		found++;
	}

	return found;
}

static int part_blk_read(struct blkdev *dev, uint64_t block, void *buf)
{
	struct part_blkdev_priv *priv = dev->private;
	return priv->parent->read(priv->parent,
				  priv->start_sector + block, buf);
}

static int part_blk_write(struct blkdev *dev, uint64_t block,
			   const void *buf)
{
	struct part_blkdev_priv *priv = dev->private;
	return priv->parent->write(priv->parent,
				   priv->start_sector + block, buf);
}

struct blkdev *partition_blkdev_create(struct blkdev *disk,
				       uint32_t start_lba,
				       uint32_t sectors)
{
	if (!disk)
		return NULL;

	struct blkdev *dev = kzalloc(sizeof(struct blkdev));
	if (!dev)
		return NULL;

	struct part_blkdev_priv *priv = kzalloc(sizeof(*priv));
	if (!priv) {
		kfree(dev);
		return NULL;
	}

	priv->parent = disk;
	priv->start_sector = start_lba;

	dev->private = priv;
	dev->block_size = disk->block_size;
	dev->total_blocks = sectors;
	dev->read = part_blk_read;
	dev->write = part_blk_write;

	return dev;
}
