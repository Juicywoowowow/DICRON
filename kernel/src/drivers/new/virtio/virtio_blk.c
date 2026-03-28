/*
 * virtio_blk.c — Virtio legacy PCI block device driver.
 *
 * Probes PCI for virtio-blk devices (vendor 0x1AF4, device 0x1001),
 * performs legacy feature negotiation, sets up virtqueue 0, reads the
 * disk capacity, and registers a struct blkdev for each device found.
 *
 * I/O is fully synchronous (polling); no interrupt handler needed.
 *
 * Supports up to VIRTIO_BLK_MAX_DEVS devices.
 */

#include "virtio_blk.h"
#include "arch/x86_64/io.h"
#include "drivers/pci/pci.h"
#include "lib/string.h"
#include "mm/kheap.h"
#include "mm/pmm.h"
#include "mm/vmm.h"
#include "virtio.h"
#include <dicron/blkdev.h>
#include <dicron/errno.h>
#include <dicron/log.h>
#include <generated/autoconf.h>

#ifdef CONFIG_VIRTIO_BLK

#define VIRTIO_BLK_MAX_DEVS 4

/* Forward declaration from virtio_blk_util.c */
int virtio_blk_submit(struct virtio_blk_dev *dev, uint32_t type,
                      uint64_t sector, void *buf);

/* ── device table ── */

static struct virtio_blk_dev devs[VIRTIO_BLK_MAX_DEVS];
static struct blkdev blkdevs[VIRTIO_BLK_MAX_DEVS];
static int ndevs;

/* ── blkdev callbacks ── */

static int vblk_read(struct blkdev *bd, uint64_t block, void *buf) {
  if (!bd || !buf)
    return -EINVAL;
  struct virtio_blk_dev *dev = (struct virtio_blk_dev *)bd->private;
  return virtio_blk_submit(dev, VIRTIO_BLK_T_IN, block, buf);
}

static int vblk_write(struct blkdev *bd, uint64_t block, const void *buf) {
  if (!bd || !buf)
    return -EINVAL;
  struct virtio_blk_dev *dev = (struct virtio_blk_dev *)bd->private;
  /*
   * Cast away const: buf is const from the blkdev API perspective,
   * but virtio_blk_submit only reads it for VIRTIO_BLK_T_OUT.
   */
  return virtio_blk_submit(dev, VIRTIO_BLK_T_OUT, block,
                           (void *)(uintptr_t)buf);
}

/* ── per-device initialisation ── */

static int virtio_blk_init_one(struct pci_device *pci) {
  if (ndevs >= VIRTIO_BLK_MAX_DEVS)
    return -ENOMEM;

  /* BAR0 must be an I/O BAR (bit 0 set) */
  if (!(pci->bar[0] & 1)) {
    klog(KLOG_WARN, "virtio-blk: BAR0 is not I/O space, skipping\n");
    return -ENODEV;
  }

  uint16_t io_base = (uint16_t)(pci->bar[0] & (uint32_t)~0x3);

  /* Enable I/O space + bus mastering in PCI command register */
  uint16_t cmd = pci_config_read16(pci->bus, pci->dev, pci->func, PCI_COMMAND);
  cmd = (uint16_t)(cmd | 0x05); /* I/O enable | bus master */
  pci_config_write16(pci->bus, pci->dev, pci->func, PCI_COMMAND, cmd);

  /* ── Virtio legacy initialisation sequence (spec §3.1.1) ── */

  outb((uint16_t)(io_base + VIRTIO_PCI_STATUS), VIRTIO_STATUS_RESET);
  outb((uint16_t)(io_base + VIRTIO_PCI_STATUS), VIRTIO_STATUS_ACK);
  outb((uint16_t)(io_base + VIRTIO_PCI_STATUS),
       (uint8_t)(VIRTIO_STATUS_ACK | VIRTIO_STATUS_DRIVER));
  outl((uint16_t)(io_base + VIRTIO_PCI_GUEST_FEATURES), 0);
  outw((uint16_t)(io_base + VIRTIO_PCI_QUEUE_SELECT), 0);

  uint16_t qsize = inw((uint16_t)(io_base + VIRTIO_PCI_QUEUE_SIZE));
  if (qsize == 0) {
    klog(KLOG_WARN, "virtio-blk: device reports queue size 0\n");
    outb((uint16_t)(io_base + VIRTIO_PCI_STATUS), VIRTIO_STATUS_FAILED);
    return -ENODEV;
  }

  /* Allocate 2 contiguous pages for the virtqueue */
  void *vq_pages = pmm_alloc_pages(1);
  if (!vq_pages) {
    klog(KLOG_ERR, "virtio-blk: OOM allocating virtqueue\n");
    outb((uint16_t)(io_base + VIRTIO_PCI_STATUS), VIRTIO_STATUS_FAILED);
    return -ENOMEM;
  }

  struct virtio_blk_dev *d = &devs[ndevs];
  memset(d, 0, sizeof(*d));
  d->io_base = io_base;

  virtq_init(&d->vq, vq_pages);

  uint64_t hhdm = vmm_get_hhdm();
  uint64_t vq_phys = (uint64_t)(uintptr_t)vq_pages - hhdm;
  outl((uint16_t)(io_base + VIRTIO_PCI_QUEUE_PFN), (uint32_t)(vq_phys >> 12));

  uint32_t cap_lo = inl((uint16_t)(io_base + VIRTIO_BLK_CFG_CAPACITY));
  uint32_t cap_hi = inl((uint16_t)(io_base + VIRTIO_BLK_CFG_CAPACITY + 4));
  d->capacity = ((uint64_t)cap_hi << 32) | (uint64_t)cap_lo;

  outb((uint16_t)(io_base + VIRTIO_PCI_STATUS),
       (uint8_t)(VIRTIO_STATUS_ACK | VIRTIO_STATUS_DRIVER |
                 VIRTIO_STATUS_DRIVER_OK));

  /* ── Register struct blkdev ── */

  struct blkdev *bd = &blkdevs[ndevs];
  memset(bd, 0, sizeof(*bd));
  bd->private = d;
  bd->block_size = 512;
  bd->total_blocks = d->capacity;
  bd->read = vblk_read;
  bd->write = vblk_write;

  d->blkdev = bd;

  klog(KLOG_INFO, "virtio-blk[%d]: io=0x%x qsize=%u capacity=%llu sectors\n",
       ndevs, (unsigned)io_base, (unsigned)qsize,
       (unsigned long long)d->capacity);

  ndevs++;
  return 0;
}

/* ── public API ── */

void virtio_blk_init(void) {
#ifndef CONFIG_PCI
  klog(KLOG_WARN, "virtio-blk: PCI support required but not compiled in\n");
  return;
#else
  /*
   * Scan all PCI devices for virtio-blk (0x1AF4:0x1001).
   * There may be more than one in a QEMU configuration.
   */
  for (int i = 0; i < pci_device_count(); i++) {
    struct pci_device *pci = pci_get_device(i);
    if (!pci)
      continue;
    if (pci->vendor_id != VIRTIO_VENDOR_ID ||
        pci->device_id != VIRTIO_BLK_DEVICE_ID)
      continue;
    (void)virtio_blk_init_one(pci);
  }

  if (ndevs == 0)
    klog(KLOG_INFO, "virtio-blk: no devices found\n");
#endif
}

int virtio_blk_count(void) { return ndevs; }

struct blkdev *virtio_blk_get_device(int idx) {
  if (idx < 0 || idx >= ndevs)
    return NULL;
  return devs[idx].blkdev;
}

#endif /* CONFIG_VIRTIO_BLK */
