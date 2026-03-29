#ifndef _DICRON_DRIVERS_SATA_H
#define _DICRON_DRIVERS_SATA_H

#include <stdint.h>
#include <generated/autoconf.h>

struct blkdev;

#ifdef CONFIG_SATA

#define SATA_MAX_DEVICES	8
#define SATA_SECTOR_SIZE	512

struct sata_device {
	int present;
	int port_index;
	uint64_t total_sectors;
	char model[41];
	char serial[21];
};

void               sata_init(void);
int                sata_drive_count(void);
struct sata_device *sata_get_drive(int index);

int sata_read(struct sata_device *dev, uint64_t lba,
	      uint32_t count, void *buf);
int sata_write(struct sata_device *dev, uint64_t lba,
	       uint32_t count, const void *buf);

struct blkdev *sata_blkdev_create(struct sata_device *dev);

#else

static inline void sata_init(void) {}
static inline int  sata_drive_count(void) { return 0; }

#endif /* CONFIG_SATA */

#endif
