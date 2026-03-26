ATA/IDE Driver
===============

1. Overview
-----------

The ATA (Advanced Technology Attachment) driver provides access to
IDE/ATA disk drives. It supports PIO (Programmed I/O) mode for reading
and writing sectors.

2. I/O Ports
------------

Primary Channel:
  0x1F0 - Data Register
  0x1F1 - Error/Features Register
  0x1F2 - Sector Count
  0x1F3 - LBA Low
  0x1F4 - LBA Mid
  0x1F5 - LBA High
  0x1F6 - Drive/Head Select
  0x1F7 - Status/Command

Secondary Channel:
  0x170 - Data Register
  0x171 - Error/Features Register
  0x172 - Sector Count
  0x173 - LBA Low
  0x174 - LBA Mid
  0x175 - LBA High
  0x176 - Drive/Head Select
  0x177 - Status/Command

3. Status Register Bits
------------------------

  0x80 - BSY (Busy)
  0x40 - DRDY (Drive Ready)
  0x20 - DF (Device Fault)
  0x10 - DSC (Drive Seek Complete)
  0x08 - DRQ (Data Request)
  0x04 - CORR (Corrected)
  0x02 - IDX (Index)
  0x01 - ERR (Error)

4. Commands
-----------

  0xEC - IDENTIFY      - Get drive information
  0x20 - READ SECTORS - Read sector(s) PIO
  0x30 - WRITE SECTORS - Write sector(s) PIO
  0xE7 - CACHE FLUSH  - Flush write cache

5. Drive Structure
------------------

  struct ata_drive {
      int present;          - Drive detected
      int bus;              - 0=primary, 1=secondary
      int drive;            - 0=master, 1=slave
      uint16_t io_base;     - I/O base port
      uint16_t ctrl_base;   - Control port
      uint8_t drive_sel;   - Drive select value
      uint32_t lba28_sectors; - 28-bit LBA capacity
      int lba48;           - 48-bit LBA supported
      uint64_t lba48_sectors; - 48-bit LBA capacity
      char model[41];      - Drive model string
  };

6. API
-----

  void ata_init(void)
      - Initialize ATA driver, detect drives

  struct ata_drive *ata_get_drive(int index)
      - Get drive by index

  int ata_drive_count(void)
      - Get number of detected drives

  int ata_pio_read(struct ata_drive *drv, uint64_t lba,
                   uint32_t count, void *buf)
      - Read sectors using PIO mode

  int ata_pio_write(struct ata_drive *drv, uint64_t lba,
                    uint32_t count, const void *buf)
      - Write sectors using PIO mode

  struct blkdev *ata_blkdev_create(struct ata_drive *drv)
      - Create block device for ATA drive

7. Partition Support
--------------------

The partition.c code handles MBR partitioning:
  - MBR located at LBA 0
  - 4 primary partitions maximum
  - Partition entries at offset 0x1BE
  - Linux partition type: 0x83

Functions:
  int partition_scan(struct blkdev *disk, struct partition_info *out, int max)
      - Scan MBR and populate partition info

  struct blkdev *partition_blkdev_create(struct blkdev *disk,
                                        uint32_t start_lba, uint32_t sectors)
      - Create virtual block device for partition
