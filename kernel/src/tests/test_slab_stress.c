#include "ktest.h"
#include "mm/slab.h"
#include "lib/string.h"
#include <stdint.h>

KTEST_REGISTER(ktest_slab_stress, "slab stress", KTEST_CAT_BOOT)
static void ktest_slab_stress(void)
{
	KTEST_BEGIN("slab stress (200 objects, pattern verify)");

	struct kmem_cache *c = kmem_cache_create("stress-32", 32, NULL, NULL);

	void *objs[200];
	for (int i = 0; i < 200; i++) {
		objs[i] = kmem_cache_alloc(c);
		KTEST_NOT_NULL(objs[i], "stress alloc");
		memset(objs[i], (uint8_t)(i & 0xFF), 32);
	}

	int ok = 1;
	for (int i = 0; i < 200; i++) {
		uint8_t *p = objs[i];
		for (int j = 0; j < 32; j++)
			if (p[j] != (uint8_t)(i & 0xFF)) ok = 0;
	}
	KTEST_TRUE(ok, "all 200 object patterns intact");

	for (int i = 0; i < 200; i++)
		kmem_cache_free(c, objs[i]);

	KTEST_EQ(c->total_allocs, 200, "200 allocs counted");
	KTEST_EQ(c->total_frees, 200, "200 frees counted");
}
