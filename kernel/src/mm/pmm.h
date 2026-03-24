#ifndef _DICRON_MM_PMM_H
#define _DICRON_MM_PMM_H

#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE	4096
#define PAGE_SHIFT	12

/*
 * Buddy allocator — orders 0..MAX_ORDER.
 * Order 0 = 1 page (4K), order N = 2^N pages.
 */
#define MAX_ORDER	12	/* up to 2^12 = 4096 pages = 16 MiB */

struct limine_memmap_response;

void    pmm_init(struct limine_memmap_response *memmap, uint64_t hhdm_offset);

/* allocate 2^order contiguous pages, returns HHDM virtual address */
void   *pmm_alloc_pages(unsigned int order);
void    pmm_free_pages(void *addr, unsigned int order);

/* convenience: single page */
static inline void *pmm_alloc_page(void)  { return pmm_alloc_pages(0); }
static inline void  pmm_free_page(void *a) { pmm_free_pages(a, 0); }

size_t  pmm_free_pages_count(void);
size_t  pmm_total_pages_count(void);
void    pmm_dump_stats(void);

/* convert between physical and HHDM virtual */
uint64_t pmm_hhdm_offset(void);

#endif
