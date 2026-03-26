Slab Allocator
==============

1. Overview
-----------

The slab allocator provides fixed-size object allocation for frequently
used kernel data structures. It is more efficient than general-purpose
allocation for objects of the same size.

2. Design
---------

The slab allocator manages caches of objects of specific sizes. Each
cache maintains three lists of slabs:

  - partial: Slabs with some free objects
  - full: Slabs with all objects allocated
  - empty: Slabs with all objects free

This design minimizes allocation latency by first allocating from
partially used slabs.

3. Slab Structure
----------------

Each slab consists of:
  - A metadata header (struct slab)
  - Space for fixed-size objects
  - A per-slab free list embedded in the object space

The slab metadata is stored at the end of the backing page, allowing
O(1) lookup of slab metadata from any object pointer.

4. API
-----

Cache management:
  struct kmem_cache *kmem_cache_create(const char *name, size_t obj_size,
                                       slab_ctor_t ctor, slab_dtor_t dtor)
      - Create a new cache for objects of obj_size
      - ctor/dtor are optional initialization/cleanup functions
      - Returns cache pointer, or NULL on failure

Allocation:
  void *kmem_cache_alloc(struct kmem_cache *cache)
      - Allocate one object from the cache
      - Returns pointer to object, or NULL on failure

Deallocation:
  void kmem_cache_free(struct kmem_cache *cache, void *obj)
      - Return an object to its cache

Diagnostics:
  void kmem_cache_dump(struct kmem_cache *cache)
      - Print statistics for a cache

  void slab_dump_all(void)
      - Print statistics for all caches

5. Built-in Caches
------------------

The kernel pre-allocates caches for common structures:
  - struct task: Task descriptors
  - struct process: Process descriptors
  - struct inode: VFS inodes
  - struct file: File descriptors
  - struct dentry: Directory entries

6. Implementation Details
--------------------------

Object alignment: Objects are aligned to cache-line boundaries for
performance. The actual allocated size (slot_size) may be larger than
the requested size (obj_size) to satisfy alignment requirements.

Free list: Each free object contains a pointer to the next free object,
forming a simple linked list. When an object is allocated, this pointer
is extracted and the next free object's address is stored in its place.

7. Advantages
-------------

The slab allocator provides:
  - O(1) allocation and deallocation
  - Reduced fragmentation
  - Cache-friendly memory access patterns
  - Optional constructor/destructor semantics

8. Comparison with Buddy Allocator
-----------------------------------

The slab allocator sits on top of the buddy allocator (PMM):
  - PMM allocates whole pages (4 KiB minimum)
  - Slab splits pages into smaller objects
  - This reduces internal fragmentation for common object sizes
