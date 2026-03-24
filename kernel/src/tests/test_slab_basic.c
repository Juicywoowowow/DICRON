#include "ktest.h"
#include "mm/slab.h"
#include "lib/string.h"

KTEST_REGISTER(ktest_slab_basic, "slab cache basic ops", KTEST_CAT_BOOT)
static void ktest_slab_basic(void)
{
	KTEST_BEGIN("slab cache basic ops");

	struct kmem_cache *c = kmem_cache_create("test-48", 48, NULL, NULL);
	KTEST_NOT_NULL(c, "cache create");
	KTEST_GE(c->slot_size, 48, "slot_size >= obj_size");
	KTEST_GT(c->objs_per_slab, 0, "objs_per_slab > 0");

	void *obj = kmem_cache_alloc(c);
	KTEST_NOT_NULL(obj, "first alloc");

	memset(obj, 0xBB, 48);
	unsigned char *p = obj;
	int ok = 1;
	for (int i = 0; i < 48; i++)
		if (p[i] != 0xBB) ok = 0;
	KTEST_TRUE(ok, "write/read pattern");

	kmem_cache_free(c, obj);

	KTEST_EQ(c->total_allocs, 1, "alloc counter");
	KTEST_EQ(c->total_frees, 1, "free counter");
}
