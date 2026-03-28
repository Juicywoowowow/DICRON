/*
 * zram.c — Compressed-RAM pool using LZO1X.
 *
 * Each slot holds a kmalloc'd compressed copy of a 4 KiB page.
 * A simple slot table + spinlock manages the pool.
 */

#include "zram.h"
#include "lz4.h"
#include "pmm.h"
#include "lib/string.h"
#include <dicron/mem.h>
#include <dicron/errno.h>
#include <dicron/log.h>
#include <dicron/spinlock.h>

struct zram_slot {
	void *data;		/* compressed data (kmalloc'd) */
	size_t compressed_len;	/* bytes of compressed data */
	int used;		/* 1 = occupied, 0 = free */
};

static struct zram_slot slots[ZRAM_MAX_SLOTS];
static spinlock_t zram_lock = SPINLOCK_INIT;
static int zram_initialized;

void zram_init(void)
{
	uint64_t flags = spin_lock_irqsave(&zram_lock);

	if (zram_initialized) {
		spin_unlock_irqrestore(&zram_lock, flags);
		return;
	}

	memset(slots, 0, sizeof(slots));
	zram_initialized = 1;

	spin_unlock_irqrestore(&zram_lock, flags);

	klog(KLOG_INFO, "zram: initialized with %d slots\n", ZRAM_MAX_SLOTS);
}

static int find_free_slot(void)
{
	for (int i = 0; i < ZRAM_MAX_SLOTS; i++) {
		if (!slots[i].used)
			return i;
	}
	return -1;
}

int zram_compress_page(const void *page)
{
	if (!page)
		return -1;

	/* Allocate work memory for LZ4 compression */
	void *wrkmem = kmalloc(LZ4_WORKMEM_SIZE);
	if (!wrkmem)
		return -1;

	/* Allocate worst-case output buffer */
	size_t worst = lz4_worst_compress(PAGE_SIZE);
	void *cbuf = kmalloc(worst);
	if (!cbuf) {
		kfree(wrkmem);
		return -1;
	}

	size_t clen = worst;
	int ret = lz4_compress(page, PAGE_SIZE, cbuf, &clen, wrkmem);
	kfree(wrkmem);

	if (ret != 0) {
		kfree(cbuf);
		return -1;
	}

	/* Shrink allocation to actual compressed size */
	void *compact = kmalloc(clen);
	if (!compact) {
		kfree(cbuf);
		return -1;
	}
	memcpy(compact, cbuf, clen);
	kfree(cbuf);

	uint64_t flags = spin_lock_irqsave(&zram_lock);

	int slot = find_free_slot();
	if (slot < 0) {
		spin_unlock_irqrestore(&zram_lock, flags);
		kfree(compact);
		return -1;
	}

	slots[slot].data = compact;
	slots[slot].compressed_len = clen;
	slots[slot].used = 1;

	spin_unlock_irqrestore(&zram_lock, flags);

	return slot;
}

int zram_decompress_page(int slot, void *page)
{
	if (!page || slot < 0 || slot >= ZRAM_MAX_SLOTS)
		return -EINVAL;

	uint64_t flags = spin_lock_irqsave(&zram_lock);

	if (!slots[slot].used) {
		spin_unlock_irqrestore(&zram_lock, flags);
		return -EINVAL;
	}

	const void *cdata = slots[slot].data;
	size_t clen = slots[slot].compressed_len;

	spin_unlock_irqrestore(&zram_lock, flags);

	size_t dlen = PAGE_SIZE;
	int ret = lz4_decompress(cdata, clen, page, &dlen);
	if (ret != 0)
		return ret;

	if (dlen != PAGE_SIZE)
		return -EIO;

	return 0;
}

void zram_free_slot(int slot)
{
	if (slot < 0 || slot >= ZRAM_MAX_SLOTS)
		return;

	uint64_t flags = spin_lock_irqsave(&zram_lock);

	if (slots[slot].used) {
		kfree(slots[slot].data);
		slots[slot].data = NULL;
		slots[slot].compressed_len = 0;
		slots[slot].used = 0;
	}

	spin_unlock_irqrestore(&zram_lock, flags);
}

int zram_used_slots(void)
{
	int count = 0;
	uint64_t flags = spin_lock_irqsave(&zram_lock);

	for (int i = 0; i < ZRAM_MAX_SLOTS; i++) {
		if (slots[i].used)
			count++;
	}

	spin_unlock_irqrestore(&zram_lock, flags);
	return count;
}
