/*
 * virtio_blk_util.c — Synchronous virtio-blk request submission.
 *
 * Builds a three-descriptor chain for each block I/O request:
 *   desc[0] — virtio_blk_req header (device reads)
 *   desc[1] — data buffer           (device reads for OUT, writes for IN)
 *   desc[2] — status byte           (device writes)
 *
 * After placing the chain head in the available ring and notifying
 * the device, we busy-poll the used ring until the device signals
 * completion.  No interrupt handler is required.
 */

#include "virtio.h"
#include "virtio_blk.h"
#include <dicron/blkdev.h>
#include "arch/x86_64/io.h"
#include "mm/vmm.h"
#include <dicron/errno.h>
#include <dicron/log.h>
#include <generated/autoconf.h>

#ifdef CONFIG_VIRTIO_BLK

/*
 * virtio_blk_submit — submit one synchronous block request.
 *
 *   dev    — virtio-blk device
 *   type   — VIRTIO_BLK_T_IN (read) or VIRTIO_BLK_T_OUT (write)
 *   sector — 512-byte LBA to access
 *   buf    — kernel virtual address of the data buffer (block_size bytes)
 *
 * Returns 0 on success, -EIO on device error or timeout.
 */
int virtio_blk_submit(struct virtio_blk_dev *dev, uint32_t type,
		      uint64_t sector, void *buf)
{
	uint64_t hhdm    = vmm_get_hhdm();
	uint16_t slot;

	if (!dev || !buf)
		return -EINVAL;

	/* Populate scratch: request header + clear status sentinel */
	dev->scratch.req.type     = type;
	dev->scratch.req.reserved = 0;
	dev->scratch.req.sector   = sector;
	dev->scratch.status       = 0xFF; /* sentinel — device overwrites with result */

	uint64_t req_phys    = (uint64_t)(uintptr_t)&dev->scratch.req    - hhdm;
	uint64_t buf_phys    = (uint64_t)(uintptr_t)buf                  - hhdm;
	uint64_t status_phys = (uint64_t)(uintptr_t)&dev->scratch.status - hhdm;

	/* desc[0]: request header — device reads this */
	dev->vq.desc[0].addr  = req_phys;
	dev->vq.desc[0].len   = (uint32_t)sizeof(struct virtio_blk_req);
	dev->vq.desc[0].flags = VIRTQ_DESC_F_NEXT;
	dev->vq.desc[0].next  = 1;

	/* desc[1]: data buffer — device writes (IN) or reads (OUT) */
	dev->vq.desc[1].addr  = buf_phys;
	dev->vq.desc[1].len   = (uint32_t)dev->blkdev->block_size;
	dev->vq.desc[1].flags = (uint16_t)(VIRTQ_DESC_F_NEXT |
		(type == VIRTIO_BLK_T_IN ? VIRTQ_DESC_F_WRITE : 0u));
	dev->vq.desc[1].next  = 2;

	/* desc[2]: status byte — device always writes this */
	dev->vq.desc[2].addr  = status_phys;
	dev->vq.desc[2].len   = 1;
	dev->vq.desc[2].flags = VIRTQ_DESC_F_WRITE;
	dev->vq.desc[2].next  = 0;

	/* Put desc-chain head (index 0) into the available ring */
	slot = dev->vq.avail->idx % (uint16_t)VIRTQ_SIZE;
	dev->vq.avail->ring[slot] = 0;

	/* Ensure descriptor writes are visible before bumping avail->idx */
	__sync_synchronize();

	dev->vq.avail->idx++;

	__sync_synchronize();

	/* Notify the device: kick queue 0 */
	outw((uint16_t)(dev->io_base + VIRTIO_PCI_QUEUE_NOTIFY), 0);

	/* Poll the used ring until the device signals completion */
	volatile struct virtq_used *used =
		(volatile struct virtq_used *)(void *)dev->vq.used;

	for (uint32_t limit = 0; used->idx == dev->vq.last_used; limit++) {
		__asm__ volatile("pause");
		if (limit > 10000000U) {
			klog(KLOG_ERR,
			     "virtio-blk: timeout on used ring (sector %llu)\n",
			     (unsigned long long)sector);
			return -EIO;
		}
	}

	dev->vq.last_used++;

	if (dev->scratch.status != VIRTIO_BLK_S_OK) {
		klog(KLOG_ERR,
		     "virtio-blk: request failed (status %u, sector %llu)\n",
		     (unsigned)dev->scratch.status,
		     (unsigned long long)sector);
		return -EIO;
	}

	return 0;
}

#endif /* CONFIG_VIRTIO_BLK */
