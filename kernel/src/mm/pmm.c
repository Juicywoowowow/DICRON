#include "pmm.h"
#include "lib/string.h"
#include "limine.h"
#include <dicron/panic.h>
#include <dicron/io.h>
#include <dicron/spinlock.h>
#include <generated/autoconf.h>

#ifdef CONFIG_SWAP
#include "swap.h"
#endif

/*
 * Buddy page allocator.
 *
 * Free lists per order (0..MAX_ORDER). Each entry is a free_page node
 * stored at the start of the free block itself (no external metadata).
 *
 * alloc(order):
 *   - If free_list[order] is non-empty, pop and return.
 *   - Otherwise, alloc(order+1) and split: return one half, free the other.
 *
 * free(addr, order):
 *   - Compute buddy address. If buddy is free at same order, coalesce
 *     into order+1. Repeat upward.
 *   - Otherwise, push onto free_list[order].
 *
 * Buddy detection uses a bitmap: one bit per page tracks "is this page
 * the allocated half of a split pair?" XOR-toggle on alloc/free lets us
 * detect when both halves are free for coalescing.
 */

struct free_page {
	struct free_page *next;
};

#define MAX_PHYS_PAGES	(512 * 1024)	/* up to 2 GiB */

static struct free_page *free_lists[MAX_ORDER + 1];
static uint8_t buddy_bitmap[MAX_PHYS_PAGES / 8];
static uint64_t hhdm;
static size_t free_count;
static size_t total_count;
static spinlock_t pmm_lock = SPINLOCK_INIT;

/* bitmap ops — one bit per page */
static inline void bm_flip(size_t pfn)
{
	buddy_bitmap[pfn / 8] = (uint8_t)(buddy_bitmap[pfn / 8] ^ (1u << (pfn % 8)));
}

static inline int bm_test(size_t pfn)
{
	return (buddy_bitmap[pfn / 8] >> (pfn % 8)) & 1;
}

static inline void *pfn_to_virt(size_t pfn)
{
	return (void *)(hhdm + ((uint64_t)pfn << PAGE_SHIFT));
}

static inline size_t virt_to_pfn(void *addr)
{
	return (size_t)(((uint64_t)addr - hhdm) >> PAGE_SHIFT);
}

/* push a block onto free_list[order] */
static void list_push(unsigned int order, void *addr)
{
	struct free_page *fp = (struct free_page *)addr;
	fp->next = free_lists[order];
	free_lists[order] = fp;
}

/* pop a block from free_list[order], NULL if empty */
static void *list_pop(unsigned int order)
{
	struct free_page *fp = free_lists[order];
	if (!fp)
		return NULL;
	free_lists[order] = fp->next;
	return (void *)fp;
}

/* remove a specific block from free_list[order] */
static int list_remove(unsigned int order, void *addr)
{
	struct free_page **pp = &free_lists[order];
	while (*pp) {
		if ((void *)*pp == addr) {
			*pp = (*pp)->next;
			return 1;
		}
		pp = &(*pp)->next;
	}
	return 0;
}

/* buddy address for a block at pfn of given order */
static size_t buddy_pfn(size_t pfn, unsigned int order)
{
	return pfn ^ ((size_t)1 << order);
}

void pmm_init(struct limine_memmap_response *memmap, uint64_t hhdm_offset)
{
	KASSERT(memmap != NULL);

	hhdm = hhdm_offset;
	free_count = 0;
	total_count = 0;

	memset(buddy_bitmap, 0, sizeof(buddy_bitmap));
	for (unsigned int i = 0; i <= MAX_ORDER; i++)
		free_lists[i] = NULL;

	/*
	 * Add usable regions directly at the largest aligned order.
	 *
	 * For each region, walk forward consuming the largest
	 * power-of-2 aligned block that fits in the remaining space.
	 */
	for (uint64_t i = 0; i < memmap->entry_count; i++) {
		struct limine_memmap_entry *e = memmap->entries[i];

		if (e->type != LIMINE_MEMMAP_USABLE)
			continue;

		uint64_t base = (e->base + PAGE_SIZE - 1) & ~((uint64_t)PAGE_SIZE - 1);
		uint64_t top  = (e->base + e->length) & ~((uint64_t)PAGE_SIZE - 1);

		uint64_t addr = base;
		while (addr < top) {
			size_t pfn = (size_t)(addr >> PAGE_SHIFT);
			if (pfn >= MAX_PHYS_PAGES)
				break;

			/* find largest order: aligned and fits */
			unsigned int order = 0;
			while (order < MAX_ORDER) {
				size_t next_size = (size_t)1 << (order + 1);
				if (pfn & (next_size - 1))
					break;
				uint64_t block_end = addr + (next_size << PAGE_SHIFT);
				if (block_end > top)
					break;
				size_t end_pfn = pfn + next_size;
				if (end_pfn > MAX_PHYS_PAGES)
					break;
				order++;
			}

			size_t block_pages = (size_t)1 << order;
			list_push(order, pfn_to_virt(pfn));
			free_count += block_pages;
			total_count += block_pages;

			addr += block_pages << PAGE_SHIFT;
		}
	}
}

void *pmm_alloc_pages(unsigned int order)
{
	KASSERT(order <= MAX_ORDER);

	uint64_t flags = spin_lock_irqsave(&pmm_lock);

	/* find smallest available order >= requested */
	for (unsigned int o = order; o <= MAX_ORDER; o++) {
		void *block = list_pop(o);
		if (!block)
			continue;

		/* split down to requested order */
		while (o > order) {
			o--;
			size_t pfn = virt_to_pfn(block);
			size_t bpfn = pfn + ((size_t)1 << o);
			list_push(o, pfn_to_virt(bpfn));
		}

		size_t pfn = virt_to_pfn(block);
		bm_flip(pfn);
		free_count -= ((size_t)1 << order);

		spin_unlock_irqrestore(&pmm_lock, flags);
		
		return block;
	}

	spin_unlock_irqrestore(&pmm_lock, flags);

#ifdef CONFIG_SWAP
	/* OOM — try to reclaim a page via swap and retry once */
	if (swap_try_reclaim(order) != NULL) {
		return pmm_alloc_pages(order);
	}
#endif

	return NULL;
}

void pmm_free_pages(void *addr, unsigned int order)
{
	KASSERT(addr != NULL);
	KASSERT(order <= MAX_ORDER);

	uint64_t flags = spin_lock_irqsave(&pmm_lock);

	size_t pfn = virt_to_pfn(addr);
	KASSERT(pfn < MAX_PHYS_PAGES);

	/*
	 * Bitmap double-free detection.  The buddy bitmap bit is set (1)
	 * when the page is allocated.  If it is already clear (0) at free
	 * time, this is a double free — reject without modifying state.
	 */
	if (!bm_test(pfn)) {
		kio_printf("\n[!!!] KERNEL SECURITY ERROR: PMM Double free detected at %p (order %u)!\n", addr, order);
		spin_unlock_irqrestore(&pmm_lock, flags);
		return;
	}

	bm_flip(pfn);
	free_count += ((size_t)1 << order);

	/* coalesce with buddy if possible */
	while (order < MAX_ORDER) {
		size_t bpfn = buddy_pfn(pfn, order);
		if (bpfn >= MAX_PHYS_PAGES)
			break;

		/* buddy must be free at this order (bit cleared = not split) */
		if (bm_test(bpfn))
			break;

		/* try to remove buddy from free list */
		if (!list_remove(order, pfn_to_virt(bpfn)))
			break;

		/* merge: take lower address */
		if (bpfn < pfn)
			pfn = bpfn;
		order++;
	}

	list_push(order, pfn_to_virt(pfn));

	spin_unlock_irqrestore(&pmm_lock, flags);
}

size_t pmm_free_pages_count(void)
{
	return free_count;
}

size_t pmm_total_pages_count(void)
{
	return total_count;
}

uint64_t pmm_hhdm_offset(void)
{
	return hhdm;
}

void pmm_dump_stats(void)
{
	kio_printf("PMM buddy stats:\n");
	kio_printf("  total: %lu pages (%lu KiB)\n",
		   total_count, total_count * 4);
	kio_printf("  free:  %lu pages (%lu KiB)\n",
		   free_count, free_count * 4);
	for (unsigned int i = 0; i <= MAX_ORDER; i++) {
		size_t count = 0;
		struct free_page *fp = free_lists[i];
		while (fp) {
			count++;
			fp = fp->next;
		}
		if (count > 0)
			kio_printf("  order %2u (%5lu KiB): %lu blocks\n",
				   i, ((size_t)1 << i) * 4, count);
	}
}
