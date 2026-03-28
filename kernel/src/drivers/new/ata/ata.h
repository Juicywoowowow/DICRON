#ifndef _DICRON_DRIVERS_ATA_H
#define _DICRON_DRIVERS_ATA_H

#include <stdint.h>

/* ── Primary bus I/O ports ── */
#define ATA_PRI_DATA        0x1F0
#define ATA_PRI_ERROR       0x1F1
#define ATA_PRI_SECT_COUNT  0x1F2
#define ATA_PRI_LBA_LO      0x1F3
#define ATA_PRI_LBA_MID     0x1F4
#define ATA_PRI_LBA_HI      0x1F5
#define ATA_PRI_DRIVE_HEAD   0x1F6
#define ATA_PRI_STATUS       0x1F7
#define ATA_PRI_COMMAND      0x1F7
#define ATA_PRI_ALT_STATUS   0x3F6
#define ATA_PRI_CTRL         0x3F6

/* ── Secondary bus I/O ports ── */
#define ATA_SEC_DATA        0x170
#define ATA_SEC_ERROR       0x171
#define ATA_SEC_SECT_COUNT  0x172
#define ATA_SEC_LBA_LO      0x173
#define ATA_SEC_LBA_MID     0x174
#define ATA_SEC_LBA_HI      0x175
#define ATA_SEC_DRIVE_HEAD   0x176
#define ATA_SEC_STATUS       0x177
#define ATA_SEC_COMMAND      0x177
#define ATA_SEC_ALT_STATUS   0x376
#define ATA_SEC_CTRL         0x376

/* ── Status register bits ── */
#define ATA_SR_BSY   0x80
#define ATA_SR_DRDY  0x40
#define ATA_SR_DF    0x20
#define ATA_SR_DSC   0x10
#define ATA_SR_DRQ   0x08
#define ATA_SR_CORR  0x04
#define ATA_SR_IDX   0x02
#define ATA_SR_ERR   0x01

/* ── Commands ── */
#define ATA_CMD_IDENTIFY      0xEC
#define ATA_CMD_READ_SECTORS  0x20
#define ATA_CMD_WRITE_SECTORS 0x30
#define ATA_CMD_CACHE_FLUSH   0xE7

/* ── Drive select values ── */
#define ATA_DRIVE_MASTER  0xE0
#define ATA_DRIVE_SLAVE   0xF0

/* ── Sector size ── */
#define ATA_SECTOR_SIZE   512

/* ── Max drives ── */
#define ATA_MAX_DRIVES    4

/* ── Drive state ── */
struct ata_drive {
	int present;
	int bus;          /* 0 = primary, 1 = secondary */
	int drive;        /* 0 = master, 1 = slave */
	uint16_t io_base;
	uint16_t ctrl_base;
	uint8_t drive_sel; /* ATA_DRIVE_MASTER or ATA_DRIVE_SLAVE */
	uint32_t lba28_sectors;
	int lba48;
	uint64_t lba48_sectors;
	char model[41];
	
	int dma;          /* 1 if DMA is configured */
	uint16_t bmr_base; /* Bus Master Register base for this channel */
	uint32_t *prd;    /* Virtual address of the PRD table */
	uint32_t prd_phys;/* Physical address of the PRD table */
};

/* ── ata_detect.c ── */
void ata_init(void);
struct ata_drive *ata_get_drive(int index);
int ata_drive_count(void);

/* ── ata_io.c (formerly ata_pio.c) ── */
int ata_read(struct ata_drive *drv, uint64_t lba, uint32_t count, void *buf);
int ata_write(struct ata_drive *drv, uint64_t lba, uint32_t count, const void *buf);

#define ata_pio_read ata_read
#define ata_pio_write ata_write

/* ── ata_blkdev.c ── */
struct blkdev;
struct blkdev *ata_blkdev_create(struct ata_drive *drv);

/* ── partition.c ── */
#define MBR_PART_TYPE_LINUX 0x83

struct mbr_entry {
	uint8_t  status;
	uint8_t  chs_first[3];
	uint8_t  type;
	uint8_t  chs_last[3];
	uint32_t lba_start;
	uint32_t sector_count;
} __attribute__((packed));

struct partition_info {
	int valid;
	uint8_t type;
	uint32_t start_lba;
	uint32_t sector_count;
};

int partition_scan(struct blkdev *disk, struct partition_info *out, int max);
struct blkdev *partition_blkdev_create(struct blkdev *disk,
				       uint32_t start_lba,
				       uint32_t sectors);

#endif
