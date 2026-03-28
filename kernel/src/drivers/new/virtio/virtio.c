/*
 * virtio.c — Virtio common virtqueue initialisation.
 *
 * Sets up the descriptor table, available ring, and used ring inside
 * two contiguous PMM pages whose layout matches the legacy virtio spec.
 */

#include "virtio.h"
#include "mm/pmm.h"
#include "lib/string.h"
#include <generated/autoconf.h>

#ifdef CONFIG_VIRTIO_BLK

void virtq_init(struct virtq *vq, void *raw_pages)
{
	uint8_t *base = (uint8_t *)raw_pages;

	memset(base, 0, (size_t)2 * PAGE_SIZE);

	/* Descriptor table at offset 0 */
	vq->desc = (struct virtq_desc *)(void *)base;

	/* Available ring immediately after the descriptor table */
	vq->avail = (struct virtq_avail *)(void *)
		(base + (size_t)VIRTQ_SIZE * sizeof(struct virtq_desc));

	/* Used ring at the start of page 1 (page-aligned, as required by spec) */
	vq->used = (struct virtq_used *)(void *)(base + PAGE_SIZE);

	vq->last_used = 0;
	vq->raw       = raw_pages;
}

#endif /* CONFIG_VIRTIO_BLK */
