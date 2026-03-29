/*
 * swap.c — Disk swap backend.
 *
 * Uses the tail of an ATA disk for page-sized swap slots.
 * Each slot is 8 sectors (4 KiB = one page).
 *
 * Slot bitmap: uint64_t (1 bit per slot, max 64 slots), spinlock-protected.
 */

#include "swap.h"
#include "lib/string.h"
#include "pmm.h"
#include <dicron/blkdev.h>
#include <dicron/errno.h>
#include <dicron/log.h>
#include <dicron/spinlock.h>
#include <generated/autoconf.h>

#ifdef CONFIG_VIRTIO_BLK
#include "drivers/new/virtio/virtio_blk.h"
#endif

static struct blkdev *swap_disk;
static uint64_t swap_start_block; /* first block of swap area */
static uint64_t swap_bitmap;      /* 1 = in-use, 0 = free */
static spinlock_t swap_lock = SPINLOCK_INIT;
static int swap_disk_enabled;
static int swap_pf_available;

void swap_init(struct blkdev *disk) {
  swap_pf_available = 1;

  if (!disk) {
#ifdef CONFIG_VIRTIO_BLK
    /*
     * No explicit disk supplied — try the first virtio-blk device.
     * virtio_blk_init() must have been called before swap_init().
     */
    if (virtio_blk_count() > 0) {
      disk = virtio_blk_get_device(0);
      if (disk)
        klog(KLOG_INFO, "swap: auto-selected virtio-blk[0] as swap disk\n");
    }
#endif
    if (!disk) {
      klog(KLOG_INFO, "swap: no disk available — ZRAM-only mode\n");
      swap_disk_enabled = 0;
      return;
    }
  }

  /* Need at least SWAP_MAX_SLOTS * SWAP_SECTORS_PER_PAGE blocks */
  uint64_t needed = (uint64_t)SWAP_MAX_SLOTS * SWAP_SECTORS_PER_PAGE;
  if (disk->total_blocks < needed) {
    klog(KLOG_WARN,
         "swap: disk too small for swap area "
         "(%llu blocks, need %llu)\n",
         (unsigned long long)disk->total_blocks, (unsigned long long)needed);
    swap_disk_enabled = 0;
    return;
  }

  swap_disk = disk;
  swap_start_block = disk->total_blocks - needed;
  swap_bitmap = 0;
  swap_disk_enabled = 1;

  klog(KLOG_DEBUG, "swap: disk swap enabled — %d slots at block %llu\n",
       SWAP_MAX_SLOTS, (unsigned long long)swap_start_block);
}

int swap_is_enabled(void) { return swap_disk_enabled; }

int swap_has_pf_support(void) { return swap_pf_available; }

static int alloc_slot(void) {
  for (int i = 0; i < SWAP_MAX_SLOTS; i++) {
    if (!(swap_bitmap & (1ULL << i))) {
      swap_bitmap |= (1ULL << i);
      return i;
    }
  }
  return -1;
}

static void free_slot_bitmap(int slot) {
  if (slot >= 0 && slot < SWAP_MAX_SLOTS)
    swap_bitmap &= ~(1ULL << slot);
}

int swap_page_out(const void *page) {
  if (!page || !swap_disk_enabled || !swap_disk)
    return -1;

  uint64_t flags = spin_lock_irqsave(&swap_lock);

  int slot = alloc_slot();
  if (slot < 0) {
    spin_unlock_irqrestore(&swap_lock, flags);
    return -1;
  }

  /* Write page across SWAP_SECTORS_PER_PAGE sectors */
  uint64_t base_block =
      swap_start_block + (uint64_t)slot * SWAP_SECTORS_PER_PAGE;
  const uint8_t *data = (const uint8_t *)page;

  for (int s = 0; s < SWAP_SECTORS_PER_PAGE; s++) {
    int ret = swap_disk->write(swap_disk, base_block + (uint64_t)s,
                               data + (size_t)s * swap_disk->block_size);
    if (ret != 0) {
      free_slot_bitmap(slot);
      spin_unlock_irqrestore(&swap_lock, flags);
      klog(KLOG_ERR, "swap: disk write failed at block %llu\n",
           (unsigned long long)(base_block + (uint64_t)s));
      return -1;
    }
  }

  spin_unlock_irqrestore(&swap_lock, flags);
  return slot;
}

int swap_page_in(int slot, void *page) {
  if (!page || slot < 0 || slot >= SWAP_MAX_SLOTS)
    return -EINVAL;
  if (!swap_disk_enabled || !swap_disk)
    return -ENODEV;

  uint64_t flags = spin_lock_irqsave(&swap_lock);

  if (!(swap_bitmap & (1ULL << slot))) {
    spin_unlock_irqrestore(&swap_lock, flags);
    return -EINVAL;
  }

  uint64_t base_block =
      swap_start_block + (uint64_t)slot * SWAP_SECTORS_PER_PAGE;
  uint8_t *data = (uint8_t *)page;

  for (int s = 0; s < SWAP_SECTORS_PER_PAGE; s++) {
    int ret = swap_disk->read(swap_disk, base_block + (uint64_t)s,
                              data + (size_t)s * swap_disk->block_size);
    if (ret != 0) {
      spin_unlock_irqrestore(&swap_lock, flags);
      klog(KLOG_ERR, "swap: disk read failed at block %llu\n",
           (unsigned long long)(base_block + (uint64_t)s));
      return -EIO;
    }
  }

  spin_unlock_irqrestore(&swap_lock, flags);
  return 0;
}

void swap_free_slot(int slot) {
  if (slot < 0 || slot >= SWAP_MAX_SLOTS)
    return;

  uint64_t flags = spin_lock_irqsave(&swap_lock);
  free_slot_bitmap(slot);
  spin_unlock_irqrestore(&swap_lock, flags);
}
