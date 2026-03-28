#ifndef _DICRON_DRIVERS_VIRTIO_H
#define _DICRON_DRIVERS_VIRTIO_H

#include <stdint.h>
#include <generated/autoconf.h>

#ifdef CONFIG_VIRTIO_BLK

/* ── Virtio PCI legacy I/O register offsets (relative to BAR0 base) ── */
#define VIRTIO_PCI_HOST_FEATURES	0x00	/* 32-bit: device feature bits (R)    */
#define VIRTIO_PCI_GUEST_FEATURES	0x04	/* 32-bit: driver feature bits (W)    */
#define VIRTIO_PCI_QUEUE_PFN		0x08	/* 32-bit: queue phys addr >> 12  (W) */
#define VIRTIO_PCI_QUEUE_SIZE		0x0C	/* 16-bit: current queue depth    (R) */
#define VIRTIO_PCI_QUEUE_SELECT		0x0E	/* 16-bit: select active queue    (W) */
#define VIRTIO_PCI_QUEUE_NOTIFY		0x10	/* 16-bit: kick a queue           (W) */
#define VIRTIO_PCI_STATUS		0x12	/* 8-bit:  device status      (R/W)   */
#define VIRTIO_PCI_ISR			0x13	/* 8-bit:  ISR status             (R) */

/* Virtio device status register bits */
#define VIRTIO_STATUS_RESET		0
#define VIRTIO_STATUS_ACK		1
#define VIRTIO_STATUS_DRIVER		2
#define VIRTIO_STATUS_DRIVER_OK		4
#define VIRTIO_STATUS_FAILED		128

/* Virtqueue descriptor flags */
#define VIRTQ_DESC_F_NEXT		1	/* descriptor chains to next  */
#define VIRTQ_DESC_F_WRITE		2	/* device writes to this buf  */

/*
 * Queue depth — must be a power of two; max 32768.
 * 16 slots: desc(256) + avail(38) < 4096, used(134) fits in page 1.
 */
#define VIRTQ_SIZE			16

/* ── Virtqueue wire structures (spec-defined layout) ── */

struct virtq_desc {
	uint64_t addr;
	uint32_t len;
	uint16_t flags;
	uint16_t next;
} __attribute__((packed));

struct virtq_avail {
	uint16_t flags;
	uint16_t idx;
	uint16_t ring[VIRTQ_SIZE];
	uint16_t used_event;
} __attribute__((packed));

struct virtq_used_elem {
	uint32_t id;
	uint32_t len;
} __attribute__((packed));

struct virtq_used {
	uint16_t flags;
	uint16_t idx;
	struct virtq_used_elem ring[VIRTQ_SIZE];
	uint16_t avail_event;
} __attribute__((packed));

/* Runtime virtqueue state kept by the driver */
struct virtq {
	struct virtq_desc  *desc;	/* VIRTQ_SIZE descriptors (page 0)  */
	struct virtq_avail *avail;	/* available ring       (page 0)    */
	struct virtq_used  *used;	/* used ring            (page 1)    */
	uint16_t            last_used;	/* last consumed used-ring index    */
	void               *raw;	/* raw allocation — 2 contiguous pages */
};

/*
 * Initialise a virtqueue.
 *   vq        — virtq to initialise
 *   raw_pages — pointer to 2 contiguous PMM pages (order 1 allocation)
 *
 * Layout:
 *   offset 0       : descriptor table  (VIRTQ_SIZE × 16 bytes)
 *   offset 256     : available ring    (38 bytes)
 *   offset 4096    : used ring         (134 bytes, page-aligned per spec)
 */
void virtq_init(struct virtq *vq, void *raw_pages);

#endif /* CONFIG_VIRTIO_BLK */

#endif /* _DICRON_DRIVERS_VIRTIO_H */
