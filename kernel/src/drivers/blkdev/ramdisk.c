#include <dicron/blkdev.h>
#include <dicron/mem.h>
#include "lib/string.h"

struct ramdisk_priv {
	uint8_t *data;
	size_t size;
};

static int ramdisk_read(struct blkdev *dev, uint64_t block, void *buf)
{
	struct ramdisk_priv *priv = dev->private;
	uint64_t offset = block * dev->block_size;

	if (offset + dev->block_size > priv->size)
		return -1;

	memcpy(buf, priv->data + offset, dev->block_size);
	return 0;
}

static int ramdisk_write(struct blkdev *dev, uint64_t block, const void *buf)
{
	struct ramdisk_priv *priv = dev->private;
	uint64_t offset = block * dev->block_size;

	if (offset + dev->block_size > priv->size)
		return -1;

	memcpy(priv->data + offset, buf, dev->block_size);
	return 0;
}

struct blkdev *ramdisk_create(void *data, size_t size, size_t block_size)
{
	if (!data || size == 0 || block_size == 0)
		return NULL;

	struct blkdev *dev = kzalloc(sizeof(struct blkdev));
	if (!dev)
		return NULL;

	struct ramdisk_priv *priv = kzalloc(sizeof(struct ramdisk_priv));
	if (!priv) {
		kfree(dev);
		return NULL;
	}

	priv->data = data;
	priv->size = size;

	dev->private = priv;
	dev->block_size = block_size;
	dev->total_blocks = size / block_size;
	dev->read = ramdisk_read;
	dev->write = ramdisk_write;

	return dev;
}
