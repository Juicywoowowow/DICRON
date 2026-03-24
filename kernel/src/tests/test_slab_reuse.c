#include "ktest.h"
#include "mm/slab.h"

KTEST_REGISTER(ktest_slab_reuse, "slab free/realloc reuse", KTEST_CAT_BOOT)
static void ktest_slab_reuse(void)
{
	KTEST_BEGIN("slab free/realloc reuse");

	struct kmem_cache *c = kmem_cache_create("reuse-64", 64, NULL, NULL);

	void *objs[20];
	for (int i = 0; i < 20; i++) {
		objs[i] = kmem_cache_alloc(c);
		KTEST_NOT_NULL(objs[i], "batch alloc");
	}

	/* free first 10 */
	for (int i = 0; i < 10; i++)
		kmem_cache_free(c, objs[i]);

	/* re-alloc 10 — should reuse freed slots */
	int reused = 0;
	for (int i = 0; i < 10; i++) {
		void *p = kmem_cache_alloc(c);
		KTEST_NOT_NULL(p, "re-alloc after free");
		/* check if address matches any freed one */
		for (int j = 0; j < 10; j++)
			if (p == objs[j]) reused++;
		objs[i] = p;
	}
	KTEST_GT(reused, 0, "slots reused after free");

	for (int i = 0; i < 20; i++)
		kmem_cache_free(c, objs[i]);
}
