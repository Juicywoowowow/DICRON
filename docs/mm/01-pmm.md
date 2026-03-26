Physical Memory Manager (PMM)
=============================

1. Overview
-----------

The Physical Memory Manager (PMM) provides allocation and deallocation
of physical memory pages. It uses a buddy allocator algorithm for
efficient memory management.

2. Buddy Allocator
-------------------

The PMM implements a buddy allocator with the following characteristics:

  - Page size: 4096 bytes (4 KiB)
  - Maximum order: 12 (allocates up to 2^12 = 4096 pages = 16 MiB)
  - Order 0: 1 page (4 KiB)
  - Order 1: 2 pages (8 KiB)
  - Order N: 2^N pages

The buddy allocator splits memory into power-of-two sized blocks called
buddy pairs. When a block is allocated, its buddy remains available for
future splitting. When freed, the allocator attempts to coalesce with
its buddy if both are free.

3. Initialization
-----------------

The PMM is initialized by pmm_init() which receives the memory map from
the Limine bootloader. The memory map contains entries describing
available memory regions (usable, reserved, etc.). The PMM uses only
regions marked as usable.

The PMM also receives the HHDM (Higher Half Direct Map) offset which
is used to convert physical addresses to virtual addresses in the kernel
address space.

4. API
------

  void pmm_init(struct limine_memmap_response *memmap, uint64_t hhdm_offset)
      - Initialize the PMM with the memory map from bootloader

  void *pmm_alloc_pages(unsigned int order)
      - Allocate 2^order contiguous pages
      - Returns virtual address (HHDM mapped), or NULL on failure

  void pmm_free_pages(void *addr, unsigned int order)
      - Free 2^order pages starting at addr

  void *pmm_alloc_page(void)
      - Convenience function for single page allocation

  void pmm_free_page(void *addr)
      - Convenience function for single page deallocation

  size_t pmm_free_pages_count(void)
      - Return number of free pages

  size_t pmm_total_pages_count(void)
      - Return total number of pages managed by PMM

  uint64_t pmm_hhdm_offset(void)
      - Return the HHDM offset for physical-to-virtual conversion

  void pmm_dump_stats(void)
      - Print PMM statistics for debugging

5. Internal Structure
----------------------

Each order has a free list. The pmm_alloc_pages() function searches
starting from the requested order and falls back to higher orders if
necessary. The allocator maintains metadata in the memory it manages.

6. Usage in Kernel
------------------

The PMM is used by:
  - Virtual Memory Manager (VMM) for allocating page tables
  - Kernel heap (kheap) for large allocations
  - Slab allocator for object allocation
  - Process creation for allocating user page tables

7. Configuration
----------------

The PMM size depends on available physical memory. It automatically
detects usable memory from the Limine memory map.
