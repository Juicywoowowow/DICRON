#ifndef _DICRON_MM_SWAP_H
#define _DICRON_MM_SWAP_H

#include <stddef.h>
#include <stdint.h>

struct blkdev;

/*
 * Unified swap subsystem.
 *
 * Reclaim priority:
 *   1. ZRAM (compress page into kernel heap)
 *   2. Disk swap (write page to ATA tail-of-disk slots)
 *   3. Give up (return NULL, no panic)
 *
 * Page fault transparency is a future extension (CONFIG_SWAP_PAGE_FAULT).
 * For now, swap is cooperative — registered pages must not be accessed
 * after eviction.
 */

/* Disk swap layout */
#define SWAP_MAX_SLOTS		64
#define SWAP_SECTORS_PER_PAGE	8	/* 8 × 512 = 4096 = PAGE_SIZE */

struct idt_frame;

/*
 * Swap PTE encoding.
 *
 * When a mapped page is evicted, its PTE is left not-present (bit 0 = 0)
 * but carries enough information to restore the page on a subsequent fault:
 *
 *   bit  0  = 0  — not present (CPU never interprets the rest)
 *   bit  1  = 1  — SWAP_PTE_MARKER: this PTE holds swap info
 *   bit  2       — SWAP_PTE_DISK: 0 = ZRAM slot, 1 = disk slot
 *   bits 8:3     — SWAP_PTE_SLOT: 6-bit slot index (0..63)
 */
#define SWAP_PTE_MARKER		(1ULL << 1)
#define SWAP_PTE_DISK		(1ULL << 2)
#define SWAP_PTE_SLOT_SHIFT	3
#define SWAP_PTE_SLOT_MASK	0x3FULL

static inline uint64_t swap_pte_encode(int disk, int slot)
{
	return SWAP_PTE_MARKER
	     | (disk ? SWAP_PTE_DISK : 0ULL)
	     | ((uint64_t)(unsigned int)slot << SWAP_PTE_SLOT_SHIFT);
}

static inline int swap_pte_is_swap(uint64_t pte)
{
	/* Present bit clear AND our marker bit set */
	return !(pte & 1ULL) && !!(pte & SWAP_PTE_MARKER);
}

static inline int swap_pte_slot(uint64_t pte)
{
	return (int)((pte >> SWAP_PTE_SLOT_SHIFT) & SWAP_PTE_SLOT_MASK);
}

/*
 * Initialize swap subsystem.
 *   disk — block device for disk swap, or NULL to auto-detect.
 *
 * When disk is NULL the following probing order is used:
 *   1. virtio-blk[0]  — when CONFIG_VIRTIO_BLK is enabled and a device
 *                        was found by virtio_blk_init() (called first).
 *   2. ZRAM-only mode — if no disk is available.
 *
 * Disk swap is only enabled when the selected disk has enough blocks for
 * SWAP_MAX_SLOTS * SWAP_SECTORS_PER_PAGE sectors.
 */
void swap_init(struct blkdev *disk);

/* Returns 1 if swap (ZRAM or disk) is enabled. */
int swap_is_enabled(void);

/* Returns 1 if page-fault-based transparent swap is available. */
int swap_has_pf_support(void);

/*
 * Register a reclaimable page.
 *   page — virtual address of the page (must be PMM-allocated, order 0)
 * The page is added to the LRU tail for future eviction.
 * Returns 0 on success, negative errno on failure.
 */
int swap_register(void *page);

/*
 * Unregister a page from the reclaimable list.
 *   page — previously registered page
 * Returns 0 on success, -ENOENT if not found.
 */
int swap_unregister(void *page);

/*
 * Try to reclaim one page to free a PMM frame.
 *   order — PMM order requested (currently only order 0 supported)
 * Drives the ZRAM → disk eviction chain.
 * Returns a newly freed page (virtual address) on success, NULL on failure.
 */
void *swap_try_reclaim(unsigned int order);

/*
 * Write a page to a disk swap slot.
 * Returns slot index (>= 0) on success, -1 on failure.
 */
int swap_page_out(const void *page);

/*
 * Read a page back from a disk swap slot.
 *   slot — slot index returned by swap_page_out
 *   page — destination buffer (PAGE_SIZE bytes)
 * Returns 0 on success, negative errno on failure.
 */
int swap_page_in(int slot, void *page);

/*
 * Free a disk swap slot.
 */
void swap_free_slot(int slot);

/*
 * Register a reclaimable page with its virtual address and address space.
 * On eviction, the page will be unmapped and its PTE encoded with slot info
 * so that a subsequent page fault triggers automatic swap-in.
 *   page      — HHDM virtual address of the page (PMM-allocated, order 0)
 *   virt      — user virtual address at which the page is mapped
 *   pml4_phys — physical address of the PML4 for the owning address space
 * Returns 0 on success, negative errno on failure.
 */
int swap_register_mapped(void *page, uint64_t virt, uint64_t pml4_phys);

/*
 * Page fault handler hook.
 * Inspects the PTE at fault_addr in the given address space.  If the PTE
 * carries a swap marker, allocates a fresh frame, reads the page back from
 * ZRAM or disk, and remaps it before returning.
 *   fault_addr — faulting virtual address (from CR2)
 *   pml4_phys  — physical address of the current PML4 (from CR3)
 * Returns 1 if the fault was handled (execution can resume), 0 otherwise.
 * Must be called with interrupts disabled (inside the exception handler).
 */
int swap_pf_handle(uint64_t fault_addr, uint64_t pml4_phys);

#endif
