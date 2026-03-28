#ifndef _DICRON_MM_ZRAM_H
#define _DICRON_MM_ZRAM_H

#include <stddef.h>
#include <stdint.h>

/*
 * ZRAM — compressed RAM pool.
 *
 * Stores pages compressed via LZ4 into kmalloc'd heap slots.
 * Useful when pages are compressible (mostly-zero), packing more
 * pages into the same RAM.  If the heap is exhausted, compress
 * will fail and the caller should fall back to disk swap.
 */

#define ZRAM_MAX_SLOTS	64

/* Initialize the ZRAM subsystem.  Safe to call multiple times. */
void zram_init(void);

/*
 * Compress a 4 KiB page into a ZRAM slot.
 *   page — virtual address of the page to compress
 * Returns slot index (>= 0) on success, -1 if pool is full or OOM.
 */
int zram_compress_page(const void *page);

/*
 * Decompress a ZRAM slot back into a 4 KiB page.
 *   slot — slot index returned by zram_compress_page
 *   page — destination buffer (must be PAGE_SIZE bytes)
 * Returns 0 on success, negative errno on failure.
 */
int zram_decompress_page(int slot, void *page);

/*
 * Free a ZRAM slot, releasing its heap memory.
 *   slot — slot index to free
 */
void zram_free_slot(int slot);

/* Query how many slots are currently in use. */
int zram_used_slots(void);

#endif
