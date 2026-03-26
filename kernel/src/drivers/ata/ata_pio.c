#include "ata.h"
#include "arch/x86_64/io.h"
#include <dicron/log.h>
#include "lib/string.h"

static void ata_delay(uint16_t ctrl)
{
	inb(ctrl);
	inb(ctrl);
	inb(ctrl);
	inb(ctrl);
}

static int ata_wait_bsy(uint16_t ctrl)
{
	for (int i = 0; i < 100000; i++) {
		if (!(inb(ctrl) & ATA_SR_BSY))
			return 0;
	}
	return -1;
}

static int ata_wait_drq(uint16_t ctrl)
{
	for (int i = 0; i < 100000; i++) {
		uint8_t st = inb(ctrl);
		if (st & (ATA_SR_ERR | ATA_SR_DF))
			return -1;
		if (!(st & ATA_SR_BSY) && (st & ATA_SR_DRQ))
			return 0;
	}
	return -1;
}

int ata_pio_read(struct ata_drive *drv, uint64_t lba,
		 uint32_t count, void *buf)
{
	if (!drv || !drv->present || !buf || count == 0)
		return -1;

	uint16_t io = drv->io_base;
	uint16_t ctrl = drv->ctrl_base;
	uint16_t *dst = (uint16_t *)buf;

	for (uint32_t s = 0; s < count; s++) {
		uint64_t sector = lba + s;

		if (ata_wait_bsy(ctrl) != 0)
			return -1;

		outb((uint16_t)(io + 6),
		     (uint8_t)(drv->drive_sel |
		     ((sector >> 24) & 0x0F)));
		outb((uint16_t)(io + 2), 1);
		outb((uint16_t)(io + 3), (uint8_t)(sector & 0xFF));
		outb((uint16_t)(io + 4), (uint8_t)((sector >> 8) & 0xFF));
		outb((uint16_t)(io + 5), (uint8_t)((sector >> 16) & 0xFF));
		outb((uint16_t)(io + 7), ATA_CMD_READ_SECTORS);

		ata_delay(ctrl);

		if (ata_wait_drq(ctrl) != 0) {
			klog(KLOG_ERR, "ata_pio_read: DRQ timeout at "
			     "LBA %lu\n", (unsigned long)sector);
			return -1;
		}

		for (uint32_t i = 0; i < 256; i++)
			dst[s * 256 + i] = inw(io);
	}

	return 0;
}

int ata_pio_write(struct ata_drive *drv, uint64_t lba,
		  uint32_t count, const void *buf)
{
	if (!drv || !drv->present || !buf || count == 0)
		return -1;

	uint16_t io = drv->io_base;
	uint16_t ctrl = drv->ctrl_base;
	const uint16_t *src = (const uint16_t *)buf;

	for (uint32_t s = 0; s < count; s++) {
		uint64_t sector = lba + s;

		if (ata_wait_bsy(ctrl) != 0)
			return -1;

		outb((uint16_t)(io + 6),
		     (uint8_t)(drv->drive_sel |
		     ((sector >> 24) & 0x0F)));
		outb((uint16_t)(io + 2), 1);
		outb((uint16_t)(io + 3), (uint8_t)(sector & 0xFF));
		outb((uint16_t)(io + 4), (uint8_t)((sector >> 8) & 0xFF));
		outb((uint16_t)(io + 5), (uint8_t)((sector >> 16) & 0xFF));
		outb((uint16_t)(io + 7), ATA_CMD_WRITE_SECTORS);

		ata_delay(ctrl);

		if (ata_wait_drq(ctrl) != 0) {
			klog(KLOG_ERR, "ata_pio_write: DRQ timeout at "
			     "LBA %lu\n", (unsigned long)sector);
			return -1;
		}

		for (uint32_t i = 0; i < 256; i++)
			outw(io, src[s * 256 + i]);

		/* Flush cache after each sector */
		outb((uint16_t)(io + 7), ATA_CMD_CACHE_FLUSH);
		if (ata_wait_bsy(ctrl) != 0) {
			klog(KLOG_ERR, "ata_pio_write: flush timeout\n");
			return -1;
		}
	}

	return 0;
}
