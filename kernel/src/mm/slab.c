#include "slab.h"
#include "lib/string.h"
#include "pmm.h"
#include <dicron/io.h>
#include <dicron/panic.h>
#include <dicron/log.h>
#include <dicron/spinlock.h>
#include <stdint.h>

/*
 * Slab allocator implementation.
 *
 * Each slab is one PMM page. The slab metadata (struct slab) is stored
 * at the end of the page to maximize usable space. Free objects are
 * linked via a pointer stored at the start of each free slot.
 *
 * Slab lookup from an object pointer is O(1): since the slab struct
 * sits at a fixed offset (end of the page), we just mask the address
 * and add the offset.
 *
 * Slab lists are doubly-linked via pprev for O(1) unlink.
 *
 * Cache metadata (struct kmem_cache) is statically allocated from a
 * small pool (we don't have a heap yet when creating the first caches).
 */

#define MAX_CACHES 128

struct free_slot {
	struct free_slot *next;
};

static struct kmem_cache cache_pool[MAX_CACHES];
static size_t cache_pool_used;
static struct kmem_cache *all_caches;
static spinlock_t slab_lock = SPINLOCK_INIT;

static size_t align_up(size_t val, size_t align)
{
	return (val + align - 1) & ~(align - 1);
}

static size_t slab_overhead(void)
{
	return align_up(sizeof(struct slab), 8);
}

/*
 * O(1) slab lookup: given any object pointer, find its slab.
 * The slab struct is always at a fixed offset at the end of the page.
 */
static struct slab *slab_from_obj(void *obj)
{
	uint64_t page_base = (uint64_t)obj & ~((uint64_t)PAGE_SIZE - 1);
	return (struct slab *)(page_base + PAGE_SIZE - slab_overhead());
}

static void slab_push(struct slab **list, struct slab *s)
{
	s->next = *list;
	s->pprev = list;
	if (*list)
		(*list)->pprev = &s->next;
	*list = s;
}

static void slab_unlink(struct slab *s)
{
	if (s->pprev)
		*s->pprev = s->next;
	if (s->next)
		s->next->pprev = s->pprev;
	s->next = NULL;
	s->pprev = NULL;
}

static struct slab *slab_create(struct kmem_cache *cache)
{
	void *page = pmm_alloc_page();
	if (!page)
		return NULL;


	size_t overhead = slab_overhead();
	size_t usable = PAGE_SIZE - overhead;
	size_t count = usable / cache->slot_size;

	if (count == 0) {
		pmm_free_page(page);
		return NULL;
	}

	struct slab *s = (struct slab *)((uint8_t *)page + PAGE_SIZE - overhead);
	s->page = page;
	s->in_use = 0;
	s->capacity = count;
	s->free_list = NULL;
	s->next = NULL;
	s->pprev = NULL;

	for (size_t i = 0; i < count; i++) {
		void *slot = (uint8_t *)page + i * cache->slot_size;

		if (cache->ctor)
			cache->ctor(slot);

		struct free_slot *fs = (struct free_slot *)slot;
		fs->next = (struct free_slot *)s->free_list;
		s->free_list = fs;
	}

	cache->slab_count++;
	return s;
}

struct kmem_cache *kmem_cache_create(const char *name, size_t obj_size,
				     slab_ctor_t ctor, slab_dtor_t dtor)
{
	uint64_t flags = spin_lock_irqsave(&slab_lock);

	if (cache_pool_used >= MAX_CACHES) {
		klog(KLOG_ERR, "slab: cache pool exhausted (%d/%d)\n",
		     (int)cache_pool_used, MAX_CACHES);
		spin_unlock_irqrestore(&slab_lock, flags);
		return NULL;
	}

	struct kmem_cache *c = &cache_pool[cache_pool_used++];

	size_t nlen = strlen(name);
	if (nlen >= KMEM_CACHE_NAME_MAX)
		nlen = KMEM_CACHE_NAME_MAX - 1;
	memcpy(c->name, name, nlen);
	c->name[nlen] = '\0';

	c->obj_size = obj_size;
	c->slot_size = align_up(
		obj_size < sizeof(struct free_slot) ? sizeof(struct free_slot) : obj_size,
		8);

	size_t overhead = slab_overhead();
	c->objs_per_slab = (PAGE_SIZE - overhead) / c->slot_size;

	c->ctor = ctor;
	c->dtor = dtor;
	c->partial = NULL;
	c->full = NULL;
	c->empty = NULL;
	c->total_allocs = 0;
	c->total_frees = 0;
	c->slab_count = 0;

	c->next_cache = all_caches;
	all_caches = c;

	spin_unlock_irqrestore(&slab_lock, flags);
	return c;
}

void *kmem_cache_alloc(struct kmem_cache *cache)
{
	if (!cache) {
		klog(KLOG_ERR, "slab: kmem_cache_alloc called with NULL cache\n");
		return NULL;
	}

	uint64_t flags = spin_lock_irqsave(&slab_lock);

	struct slab *s = NULL;

	if (cache->partial) {
		s = cache->partial;
	} else if (cache->empty) {
		s = cache->empty;
		slab_unlink(s);
		slab_push(&cache->partial, s);
	} else {
		s = slab_create(cache);
		if (!s) {
			spin_unlock_irqrestore(&slab_lock, flags);
			return NULL;
		}
		slab_push(&cache->partial, s);
	}

	if (!s->free_list) {
		klog(KLOG_ERR, "slab: free_list is NULL in partial slab (cache: %s)\n", cache->name);
		spin_unlock_irqrestore(&slab_lock, flags);
		return NULL;
	}
	struct free_slot *slot = (struct free_slot *)s->free_list;
	s->free_list = slot->next;
	s->in_use++;
	cache->total_allocs++;

	if (s->in_use == s->capacity) {
		slab_unlink(s);
		slab_push(&cache->full, s);
	}

	spin_unlock_irqrestore(&slab_lock, flags);
	return (void *)slot;
}

void kmem_cache_free(struct kmem_cache *cache, void *obj)
{
	if (!cache) {
		klog(KLOG_ERR, "slab: kmem_cache_free called with NULL cache\n");
		return;
	}
	if (!obj) {
		klog(KLOG_ERR, "slab: kmem_cache_free called with NULL obj\n");
		return;
	}

	uint64_t flags = spin_lock_irqsave(&slab_lock);

	struct slab *s = slab_from_obj(obj);
	if (s->page != (void *)((uint64_t)obj & ~((uint64_t)PAGE_SIZE - 1))) {
		klog(KLOG_ERR, "slab: corrupt free — obj %p does not belong to slab page %p (cache: %s)\n",
		     obj, s->page, cache->name);
		spin_unlock_irqrestore(&slab_lock, flags);
		return;
	}

	int was_full = (s->in_use == s->capacity);

	/* double-free detection */
	for (struct free_slot *curr = s->free_list; curr; curr = curr->next) {
		if ((void *)curr == obj) {
			kio_printf("\n[!!!] double free detected on %p in cache [%s]!\n",
				   obj, cache->name);
			spin_unlock_irqrestore(&slab_lock, flags);
			return;
		}
	}

	if (cache->dtor)
		cache->dtor(obj);

	struct free_slot *slot = (struct free_slot *)obj;
	slot->next = (struct free_slot *)s->free_list;
	s->free_list = slot;
	s->in_use--;
	cache->total_frees++;

	if (s->in_use == 0) {
		slab_unlink(s);
		if (cache->empty) {
			pmm_free_page(s->page);
			cache->slab_count--;
		} else {
			slab_push(&cache->empty, s);
		}
	} else if (was_full) {
		slab_unlink(s);
		slab_push(&cache->partial, s);
	}

	spin_unlock_irqrestore(&slab_lock, flags);
}

void kmem_cache_dump(struct kmem_cache *cache)
{
	size_t partial_n = 0, full_n = 0, empty_n = 0;

	for (struct slab *s = cache->partial; s; s = s->next)
		partial_n++;
	for (struct slab *s = cache->full; s; s = s->next)
		full_n++;
	for (struct slab *s = cache->empty; s; s = s->next)
		empty_n++;

	kio_printf("  [%s] obj=%lu slot=%lu  per_slab=%lu  "
		   "slabs(p/f/e)=%lu/%lu/%lu  allocs=%lu frees=%lu\n",
		   cache->name, cache->obj_size, cache->slot_size,
		   cache->objs_per_slab, partial_n, full_n, empty_n,
		   cache->total_allocs, cache->total_frees);
}

void slab_dump_all(void)
{
	kio_printf("slab caches:\n");
	for (struct kmem_cache *c = all_caches; c; c = c->next_cache)
		kmem_cache_dump(c);
}
