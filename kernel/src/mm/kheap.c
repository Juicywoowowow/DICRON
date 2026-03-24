#include "kheap.h"
#include "slab.h"
#include "pmm.h"
#include "lib/string.h"
#include <dicron/panic.h>
#include <dicron/log.h>
#include <dicron/io.h>
#include <dicron/spinlock.h>
#include <stdint.h>

/*
 * Generic kernel heap built on top of slab caches.
 *
 * Power-of-2 caches: 16, 32, 64, 128, 256, 512, 1024, 2048 bytes.
 * Each kmalloc picks the smallest cache that fits (size + header).
 * Allocations > 2048 use multi-page PMM allocs directly.
 *
 * Every allocation is prefixed with a small header storing the
 * cache pointer (or NULL for page-level allocs), the user-requested
 * size, and the buddy order for large allocations.
 */

#define CACHE_COUNT	8
#define LARGE_MARKER	((struct kmem_cache *)(uintptr_t)0xDEAD)

struct alloc_hdr {
	struct kmem_cache *cache;	/* which cache, or LARGE_MARKER */
	size_t		   size;	/* user-requested size (always clean) */
	unsigned int	   order;	/* buddy order (large allocs only) */
};

static struct kmem_cache *caches[CACHE_COUNT];

static const size_t cache_sizes[CACHE_COUNT] = {
	16, 32, 64, 128, 256, 512, 1024, 2048
};

static const char *cache_names[CACHE_COUNT] = {
	"kmalloc-16",  "kmalloc-32",  "kmalloc-64",   "kmalloc-128",
	"kmalloc-256", "kmalloc-512", "kmalloc-1024", "kmalloc-2048"
};

void kheap_init(void)
{
	for (int i = 0; i < CACHE_COUNT; i++) {
		/* slot must hold alloc_hdr + payload */
		size_t slot = sizeof(struct alloc_hdr) + cache_sizes[i];
		caches[i] = kmem_cache_create(cache_names[i], slot, NULL, NULL);
		if (!caches[i]) {
			klog(KLOG_ERR, "kheap: failed to create cache '%s'\n", cache_names[i]);
		}
	}
}

static int cache_index_for(size_t size)
{
	for (int i = 0; i < CACHE_COUNT; i++) {
		if (size <= cache_sizes[i])
			return i;
	}
	return -1;
}

void *kmalloc(size_t size)
{
	if (size == 0)
		return NULL;

	int idx = cache_index_for(size);

	if (idx >= 0) {
		void *raw = kmem_cache_alloc(caches[idx]);
		if (!raw)
			return NULL;

		struct alloc_hdr *hdr = (struct alloc_hdr *)raw;
		hdr->cache = caches[idx];
		hdr->size = size;
		hdr->order = 0;
		return (void *)(hdr + 1);
	}

	/* large allocation — whole pages from buddy */
	size_t total = size + sizeof(struct alloc_hdr);
	size_t pages = (total + PAGE_SIZE - 1) / PAGE_SIZE;

	/* compute order */
	unsigned int order = 0;
	while (((size_t)1 << order) < pages)
		order++;
		
	if (order > MAX_ORDER)
		return NULL;

	void *raw = pmm_alloc_pages(order);
	if (!raw)
		return NULL;

	struct alloc_hdr *hdr = (struct alloc_hdr *)raw;
	hdr->cache = LARGE_MARKER;
	hdr->size = size;
	hdr->order = order;
	return (void *)(hdr + 1);
}

void *kzalloc(size_t size)
{
	void *p = kmalloc(size);
	if (p)
		memset(p, 0, size);
	return p;
}

void *krealloc(void *ptr, size_t new_size)
{
	if (!ptr)
		return kmalloc(new_size);
	if (new_size == 0) {
		kfree(ptr);
		return NULL;
	}

	struct alloc_hdr *hdr = (struct alloc_hdr *)ptr - 1;
	size_t old_size = hdr->size;

	/* if it already fits, skip the copy */
	if (new_size <= old_size)
		return ptr;

	void *newp = kmalloc(new_size);
	if (!newp)
		return NULL;

	memcpy(newp, ptr, old_size);
	kfree(ptr);
	return newp;
}

void kfree(void *ptr)
{
	if (!ptr)
		return;

	struct alloc_hdr *hdr = (struct alloc_hdr *)ptr - 1;

	if (hdr->cache == LARGE_MARKER) {
		pmm_free_pages((void *)hdr, hdr->order);
		return;
	}

	kmem_cache_free(hdr->cache, (void *)hdr);
}

size_t kmalloc_usable_size(void *ptr)
{
	if (!ptr)
		return 0;

	struct alloc_hdr *hdr = (struct alloc_hdr *)ptr - 1;

	if (hdr->cache == LARGE_MARKER)
		return ((size_t)1 << hdr->order) * PAGE_SIZE
		       - sizeof(struct alloc_hdr);

	return hdr->cache->obj_size - sizeof(struct alloc_hdr);
}

void kheap_dump_stats(void)
{
	kio_printf("kheap (slab-backed):\n");
	slab_dump_all();
}
