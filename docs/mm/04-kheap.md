Kernel Heap (kheap)
===================

1. Overview
-----------

The kernel heap provides dynamic memory allocation for the kernel.
It sits on top of the slab allocator and PMM, providing malloc/free
semantics for variable-sized allocations.

2. Implementation
-----------------

The kernel heap uses a simple bump allocator strategy combined with
the slab allocator for larger allocations. Small allocations are served
from the slab caches, while larger allocations use PMM directly.

3. API
-----

Standard allocation functions:
  void *kmalloc(size_t size)
      - Allocate memory of given size
      - Returns pointer, or NULL on failure

  void *kzalloc(size_t size)
      - Allocate zero-initialized memory
      - Equivalent to kmalloc followed by memset to 0

  void *krealloc(void *ptr, size_t new_size)
      - Resize an existing allocation
      - May allocate new memory and copy data
      - Returns new pointer

  void kfree(void *ptr)
      - Free previously allocated memory

  size_t kmalloc_usable_size(void *ptr)
      - Get the actual allocated size of a pointer

  void kheap_dump_stats(void)
      - Print heap statistics for debugging

4. Usage
--------

The kernel heap is used throughout the kernel for:
  - General-purpose dynamic allocation
  - Allocating VFS structures
  - File descriptor allocation
  - Process creation

5. Initialization
-----------------

The heap is initialized by kheap_init() during kernel boot. It creates
slab caches for common kernel object sizes.

6. Error Handling
-----------------

Allocation functions return NULL on failure. Callers should check
return values. In kernel panic situations, allocation failure may
trigger a kernel panic.

7. Memory Overhead
------------------

The heap maintains metadata for each allocation to track size. This
overhead is typically 8-16 bytes per allocation.
