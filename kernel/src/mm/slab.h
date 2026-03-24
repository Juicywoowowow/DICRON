#ifndef _DICRON_MM_SLAB_H
#define _DICRON_MM_SLAB_H

#include <stddef.h>

/*
 * Slab allocator — object caches backed by PMM pages.
 *
 * Each cache manages fixed-size objects. Pages are carved into slots;
 * a per-page free list tracks available slots. Caches maintain lists
 * of partial, full, and empty slabs.
 *
 * Slab lists are doubly-linked (via pprev) for O(1) unlink.
 * Slab lookup from an object pointer is O(1): the slab metadata
 * lives at a fixed offset at the end of the backing page.
 *
 * Usage:
 *   struct kmem_cache *c = kmem_cache_create("task", sizeof(struct task), NULL, NULL);
 *   struct task *t = kmem_cache_alloc(c);
 *   kmem_cache_free(c, t);
 */

typedef void (*slab_ctor_t)(void *obj);
typedef void (*slab_dtor_t)(void *obj);

struct slab {
	struct slab	*next;
	struct slab    **pprev;		/* &prev->next — enables O(1) unlink */
	void		*page;		/* base of the backing page */
	size_t		 in_use;	/* number of allocated objects */
	size_t		 capacity;	/* objects per slab */
	void		*free_list;	/* linked list of free slots */
};

#define KMEM_CACHE_NAME_MAX 32

struct kmem_cache {
	char		 name[KMEM_CACHE_NAME_MAX];
	size_t		 obj_size;	/* user-requested size */
	size_t		 slot_size;	/* aligned size (>= obj_size) */
	size_t		 objs_per_slab;
	slab_ctor_t	 ctor;
	slab_dtor_t	 dtor;

	struct slab	*partial;	/* some objects free */
	struct slab	*full;		/* all objects in use */
	struct slab	*empty;		/* all objects free */

	/* stats */
	size_t		 total_allocs;
	size_t		 total_frees;
	size_t		 slab_count;

	/* linked list of all caches (for debugging) */
	struct kmem_cache *next_cache;
};

struct kmem_cache *kmem_cache_create(const char *name, size_t obj_size,
				     slab_ctor_t ctor, slab_dtor_t dtor);
void		  *kmem_cache_alloc(struct kmem_cache *cache);
void		   kmem_cache_free(struct kmem_cache *cache, void *obj);
void		   kmem_cache_dump(struct kmem_cache *cache);

/* dump all caches */
void		   slab_dump_all(void);

#endif
