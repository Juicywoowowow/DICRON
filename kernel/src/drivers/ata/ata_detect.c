#include "ata.h"
#include "arch/x86_64/io.h"
#include <dicron/log.h>
#include "lib/string.h"
#include <generated/autoconf.h>

static struct ata_drive drives[ATA_MAX_DRIVES];
static int num_drives;

static void ata_delay(uint16_t ctrl_port)
{
	inb(ctrl_port);
	inb(ctrl_port);
	inb(ctrl_port);
	inb(ctrl_port);
}

static int ata_wait_bsy(uint16_t ctrl_port)
{
	for (int i = 0; i < 100000; i++) {
		if (!(inb(ctrl_port) & ATA_SR_BSY))
			return 0;
	}
	return -1;
}

static int ata_wait_drq(uint16_t ctrl_port)
{
	for (int i = 0; i < 100000; i++) {
		uint8_t st = inb(ctrl_port);
		if (st & (ATA_SR_ERR | ATA_SR_DF))
			return -1;
		if (!(st & ATA_SR_BSY) && (st & ATA_SR_DRQ))
			return 0;
	}
	return -1;
}

static void ata_read_model(uint16_t *ident, char *model)
{
	/* Model string is in words 27–46, byte-swapped */
	for (int i = 0; i < 20; i++) {
		model[i * 2]     = (char)(ident[27 + i] >> 8);
		model[i * 2 + 1] = (char)(ident[27 + i] & 0xFF);
	}
	model[40] = '\0';

	/* Trim trailing spaces */
	for (int i = 39; i >= 0 && model[i] == ' '; i--)
		model[i] = '\0';
}

static int ata_identify(struct ata_drive *drv)
{
	uint16_t io = drv->io_base;
	uint16_t ctrl = drv->ctrl_base;

	outb((uint16_t)(io + 6), drv->drive_sel);
	ata_delay(ctrl);

	outb((uint16_t)(io + 2), 0);
	outb((uint16_t)(io + 3), 0);
	outb((uint16_t)(io + 4), 0);
	outb((uint16_t)(io + 5), 0);

	outb((uint16_t)(io + 7), ATA_CMD_IDENTIFY);
	ata_delay(ctrl);

	uint8_t st = inb(ctrl);
	if (st == 0)
		return -1;

	if (ata_wait_bsy(ctrl) != 0)
		return -1;

	/* Non-zero LBA mid/hi means ATAPI, not ATA */
	if (inb((uint16_t)(io + 4)) != 0 || inb((uint16_t)(io + 5)) != 0)
		return -1;

	if (ata_wait_drq(ctrl) != 0)
		return -1;

	uint16_t ident[256];
	for (int i = 0; i < 256; i++)
		ident[i] = inw(io);

	drv->lba28_sectors = (uint32_t)ident[60] |
			     ((uint32_t)ident[61] << 16);

	drv->lba48 = (ident[83] & (1 << 10)) ? 1 : 0;
	if (drv->lba48) {
		drv->lba48_sectors = (uint64_t)ident[100] |
				     ((uint64_t)ident[101] << 16) |
				     ((uint64_t)ident[102] << 32) |
				     ((uint64_t)ident[103] << 48);
	}

	ata_read_model(ident, drv->model);
	drv->present = 1;
	return 0;
}

#if defined(CONFIG_ATA_PRIMARY) || defined(CONFIG_ATA_SECONDARY)
static void ata_probe_bus(uint16_t io_base, uint16_t ctrl_base, int bus)
{
	for (int d = 0; d < 2; d++) {
		if (num_drives >= ATA_MAX_DRIVES)
			return;

		struct ata_drive *drv = &drives[num_drives];
		memset(drv, 0, sizeof(*drv));
		drv->bus = bus;
		drv->drive = d;
		drv->io_base = io_base;
		drv->ctrl_base = ctrl_base;
		drv->drive_sel = (d == 0) ? ATA_DRIVE_MASTER
					  : ATA_DRIVE_SLAVE;

		if (ata_identify(drv) == 0) {
			klog(KLOG_INFO, "ata%d.%d: %s — %u sectors "
			     "(LBA48=%d)\n", bus, d, drv->model,
			     drv->lba28_sectors, drv->lba48);
			num_drives++;
		}
	}
}
#endif

void ata_init(void)
{
	num_drives = 0;

#ifdef CONFIG_ATA_PRIMARY
	ata_probe_bus(ATA_PRI_DATA, ATA_PRI_ALT_STATUS, 0);
#endif
#ifdef CONFIG_ATA_SECONDARY
	ata_probe_bus(ATA_SEC_DATA, ATA_SEC_ALT_STATUS, 1);
#endif

	klog(KLOG_INFO, "ata: %d drive(s) detected\n", num_drives);
}

struct ata_drive *ata_get_drive(int index)
{
	if (index < 0 || index >= num_drives)
		return NULL;
	return &drives[index];
}

int ata_drive_count(void)
{
	return num_drives;
}
