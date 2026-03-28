#include "arch/x86_64/io.h"
#include "ata.h"
#include "lib/string.h"
#include "mm/vmm.h"
#include <dicron/log.h>

static void ata_delay(uint16_t ctrl) {
  inb(ctrl);
  inb(ctrl);
  inb(ctrl);
  inb(ctrl);
}

static int ata_wait_bsy(uint16_t ctrl) {
  for (int i = 0; i < 100000; i++) {
    if (!(inb(ctrl) & ATA_SR_BSY))
      return 0;
  }
  return -1;
}

static int ata_wait_drq(uint16_t ctrl) {
  for (int i = 0; i < 100000; i++) {
    uint8_t st = inb(ctrl);
    if (st & (ATA_SR_ERR | ATA_SR_DF))
      return -1;
    if (!(st & ATA_SR_BSY) && (st & ATA_SR_DRQ))
      return 0;
  }
  return -1;
}

/* Fallback legacy PIO paths */
static int ata_read_pio_internal(struct ata_drive *drv, uint64_t lba,
                                 uint32_t count, void *buf) {
  uint16_t io = drv->io_base;
  uint16_t ctrl = drv->ctrl_base;
  uint16_t *dst = (uint16_t *)buf;

  for (uint32_t s = 0; s < count; s++) {
    uint64_t sector = lba + s;
    if (ata_wait_bsy(ctrl) != 0)
      return -1;

    outb((uint16_t)(io + 6),
         (uint8_t)(drv->drive_sel | ((sector >> 24) & 0x0F)));
    outb((uint16_t)(io + 2), 1);
    outb((uint16_t)(io + 3), (uint8_t)(sector & 0xFF));
    outb((uint16_t)(io + 4), (uint8_t)((sector >> 8) & 0xFF));
    outb((uint16_t)(io + 5), (uint8_t)((sector >> 16) & 0xFF));
    outb((uint16_t)(io + 7), ATA_CMD_READ_SECTORS);

    ata_delay(ctrl);

    if (ata_wait_drq(ctrl) != 0)
      return -1;
    for (uint32_t i = 0; i < 256; i++)
      dst[s * 256 + i] = inw(io);
  }
  return 0;
}

static int ata_write_pio_internal(struct ata_drive *drv, uint64_t lba,
                                  uint32_t count, const void *buf) {
  uint16_t io = drv->io_base;
  uint16_t ctrl = drv->ctrl_base;
  const uint16_t *src = (const uint16_t *)buf;

  for (uint32_t s = 0; s < count; s++) {
    uint64_t sector = lba + s;
    if (ata_wait_bsy(ctrl) != 0)
      return -1;

    outb((uint16_t)(io + 6),
         (uint8_t)(drv->drive_sel | ((sector >> 24) & 0x0F)));
    outb((uint16_t)(io + 2), 1);
    outb((uint16_t)(io + 3), (uint8_t)(sector & 0xFF));
    outb((uint16_t)(io + 4), (uint8_t)((sector >> 8) & 0xFF));
    outb((uint16_t)(io + 5), (uint8_t)((sector >> 16) & 0xFF));
    outb((uint16_t)(io + 7), ATA_CMD_WRITE_SECTORS);

    ata_delay(ctrl);

    if (ata_wait_drq(ctrl) != 0)
      return -1;
    for (uint32_t i = 0; i < 256; i++)
      outw(io, src[s * 256 + i]);

    outb((uint16_t)(io + 7), ATA_CMD_CACHE_FLUSH);
    if (ata_wait_bsy(ctrl) != 0)
      return -1;
  }
  return 0;
}

int ata_read(struct ata_drive *drv, uint64_t lba, uint32_t count, void *buf) {
  if (!drv || !drv->present || !buf || count == 0)
    return -1;

  uint64_t max_sectors = drv->lba48 ? drv->lba48_sectors : drv->lba28_sectors;
  if (lba + count > max_sectors || lba + count < lba)
    return -1;

  /* Bus Master DMA attempt */
  if (drv->dma && count <= (65536 / 512)) {
    uint64_t buf_phys = vmm_virt_to_phys((uint64_t)buf);
    uint64_t end_phys = vmm_virt_to_phys((uint64_t)buf + (count * 512 - 1));
    /* Check physical contiguity */
    if ((buf_phys & ~0xFFFULL) == (end_phys & ~0xFFFULL)) {
      drv->prd[0] = (uint32_t)buf_phys;
      drv->prd[1] = (count * 512) | 0x80000000;

      outl(drv->bmr_base + 4, drv->prd_phys);
      outb(drv->bmr_base + 2, 0x06); /* clear errors */
      outb(drv->bmr_base + 0, 0x08); /* DMA read direction */

      if (ata_wait_bsy(drv->ctrl_base) == 0) {
        outb(drv->io_base + 6,
             (uint8_t)(drv->drive_sel | ((lba >> 24) & 0x0F)));
        outb(drv->io_base + 2, (uint8_t)count);
        outb(drv->io_base + 3, (uint8_t)(lba & 0xFF));
        outb(drv->io_base + 4, (uint8_t)((lba >> 8) & 0xFF));
        outb(drv->io_base + 5, (uint8_t)((lba >> 16) & 0xFF));
        outb(drv->io_base + 7, 0xC8); /* READ DMA */

        outb(drv->bmr_base + 0, 0x09); /* start DMA read */

        for (int i = 0; i < 2000000; i++) {
          uint8_t st = inb(drv->bmr_base + 2);
          if (st & 4)
            break; /* interrupted */
          if (st & 2)
            break; /* errored */
        }

        outb(drv->bmr_base + 0, 0x00);
        uint8_t final_st = inb(drv->bmr_base + 2);
        if (!(final_st & 2) && (final_st & 4)) {
          return 0; /* DMA Success */
        }
      }
    }
  }

  return ata_read_pio_internal(drv, lba, count, buf);
}

int ata_write(struct ata_drive *drv, uint64_t lba, uint32_t count,
              const void *buf) {
  if (!drv || !drv->present || !buf || count == 0)
    return -1;
    
  uint64_t max_sectors = drv->lba48 ? drv->lba48_sectors : drv->lba28_sectors;
  if (lba + count > max_sectors || lba + count < lba)
    return -1;
  if (drv->dma && count <= (65536 / 512)) {
    uint64_t buf_phys = vmm_virt_to_phys((uint64_t)buf);
    uint64_t end_phys = vmm_virt_to_phys((uint64_t)buf + (count * 512 - 1));
    /* Check physical contiguity */
    if ((buf_phys & ~0xFFFULL) == (end_phys & ~0xFFFULL)) {
      drv->prd[0] = (uint32_t)buf_phys;
      drv->prd[1] = (count * 512) | 0x80000000;

      outl(drv->bmr_base + 4, drv->prd_phys);
      outb(drv->bmr_base + 2, 0x06); /* clear errors */
      outb(drv->bmr_base + 0, 0x00); /* DMA write direction */

      if (ata_wait_bsy(drv->ctrl_base) == 0) {
        outb(drv->io_base + 6,
             (uint8_t)(drv->drive_sel | ((lba >> 24) & 0x0F)));
        outb(drv->io_base + 2, (uint8_t)count);
        outb(drv->io_base + 3, (uint8_t)(lba & 0xFF));
        outb(drv->io_base + 4, (uint8_t)((lba >> 8) & 0xFF));
        outb(drv->io_base + 5, (uint8_t)((lba >> 16) & 0xFF));
        outb(drv->io_base + 7, 0xCA); /* WRITE DMA */

        outb(drv->bmr_base + 0, 0x01); /* start DMA write */

        for (int i = 0; i < 2000000; i++) {
          uint8_t st = inb(drv->bmr_base + 2);
          if (st & 4)
            break; /* interrupted */
          if (st & 2)
            break; /* errored */
        }

        outb(drv->bmr_base + 0, 0x00);
        uint8_t final_st = inb(drv->bmr_base + 2);
        if (!(final_st & 2) && (final_st & 4)) {
          return 0; /* DMA Success */
        }
      }
    }
  }

  return ata_write_pio_internal(drv, lba, count, buf);
}
