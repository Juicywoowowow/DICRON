#ifndef _DICRON_DRIVERS_VIRTIO_BLK_H
#define _DICRON_DRIVERS_VIRTIO_BLK_H

#include <stdint.h>
#include "virtio.h"
#include <generated/autoconf.h>

struct blkdev;

#ifdef CONFIG_VIRTIO_BLK

/* Virtio PCI identifiers */
#define VIRTIO_VENDOR_ID	0x1AF4
#define VIRTIO_BLK_DEVICE_ID	0x1001	/* legacy virtio-blk */

/* Device-specific config offset inside BAR0 for virtio-blk */
#define VIRTIO_BLK_CFG_CAPACITY	0x14	/* 64-bit: total 512-byte sectors */

/* Virtio block request types */
#define VIRTIO_BLK_T_IN		0	/* read from device  */
#define VIRTIO_BLK_T_OUT	1	/* write to device   */

/* Virtio block completion status */
#define VIRTIO_BLK_S_OK		0
#define VIRTIO_BLK_S_IOERR	1
#define VIRTIO_BLK_S_UNSUPP	2

/* Virtio block request header (spec-defined, sent as first descriptor) */
struct virtio_blk_req {
	uint32_t type;
	uint32_t reserved;
	uint64_t sector;
} __attribute__((packed));

/*
 * Per-device scratch buffer: request header + status byte.
 * Embedded in virtio_blk_dev so it lives in kzalloc'd (HHDM-mapped)
 * memory and has a computable physical address for DMA.
 */
struct virtio_blk_scratch {
	struct virtio_blk_req req;	/* 16 bytes */
	uint8_t               status;	/*  1 byte  */
	uint8_t               _pad[7];	/* alignment */
};

/* Per-device driver state */
struct virtio_blk_dev {
	uint16_t                 io_base;  /* BAR0 I/O port base             */
	uint64_t                 capacity; /* total 512-byte sectors          */
	struct virtq             vq;       /* virtqueue 0                     */
	struct virtio_blk_scratch scratch; /* DMA request header + status     */
	struct blkdev           *blkdev;   /* registered blkdev               */
};

/* ── Public API ── */

/*
 * Scan PCI for virtio-blk devices (vendor 0x1AF4 / device 0x1001),
 * perform legacy feature negotiation, and register a struct blkdev
 * for each device found.
 */
void virtio_blk_init(void);

/* Return the number of virtio-blk devices successfully initialised. */
int virtio_blk_count(void);

/*
 * Return the blkdev for device at index idx (0-based).
 * Returns NULL if idx is out of range or no devices were found.
 */
struct blkdev *virtio_blk_get_device(int idx);

#else  /* !CONFIG_VIRTIO_BLK */

static inline void virtio_blk_init(void) {}
static inline int  virtio_blk_count(void) { return 0; }
static inline struct blkdev *virtio_blk_get_device(int idx)
{
	(void)idx;
	return NULL;
}

#endif /* CONFIG_VIRTIO_BLK */

#endif /* _DICRON_DRIVERS_VIRTIO_BLK_H */
