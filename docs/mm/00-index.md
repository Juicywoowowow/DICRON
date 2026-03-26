Memory Management Subsystem
===========================

1. Overview
-----------

The memory management subsystem is divided into four layers:

  Layer 1: PMM (Physical Memory Manager) - Buddy allocator
  Layer 2: VMM (Virtual Memory Manager) - Page tables
  Layer 3: Slab Allocator - Object caches
  Layer 4: Kernel Heap - General allocation

2. Page Size
------------

All memory management uses 4 KiB (4096 byte) pages:
  PAGE_SIZE = 4096
  PAGE_SHIFT = 12

3. Memory Allocation Flow
--------------------------

  User Request -> kmalloc/kfree -> Kernel Heap
                              -> Slab Cache (if sized appropriately)
                              -> PMM (for large allocations)

4. Address Space Separation
---------------------------

The kernel maintains separate page tables for:
  - Kernel space: Identity-mapped via HHDM
  - User space: Per-process, non-privileged

5. Memory Types
---------------

  Physical Memory: Managed by PMM
  Virtual Memory: Managed by VMM
  Kernel Memory: Uses HHDM mapping
  User Memory: Separate per-process

6. Page Table Management
------------------------

VMM maintains the following for each address space:
  - PML4 (Page Map Level 4)
  - PDPT (Page Directory Pointer Table)
  - PD (Page Directory)
  - PT (Page Table)

7. Kernel Memory Layout
------------------------

  0xFFFF800000000000+ - Direct mapping (physical + offset)
  - 0xFFFF800000000000-0xFFFF80FFFFFFFFFF (1 TiB)

  User space for processes:
  0x0000000000000000-0x00007FFFFFFFFFFF (128 TiB)

8. Initialization Order
------------------------

During boot, memory is initialized in order:
  1. PMM first (uses raw physical memory)
  2. VMM second (creates kernel page tables)
  3. Slab allocator third (uses PMM pages)
  4. Kernel heap last (uses slab for small, PMM for large)
