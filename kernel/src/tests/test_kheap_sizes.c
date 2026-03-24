#include "ktest.h"
#include "mm/kheap.h"
#include "lib/string.h"

KTEST_REGISTER(ktest_kheap_sizes, "kmalloc size classes", KTEST_CAT_BOOT)
static void ktest_kheap_sizes(void)
{
	KTEST_BEGIN("kmalloc size classes");

	static const size_t sizes[] = {
		1, 8, 15, 16, 31, 32, 63, 64,
		127, 128, 255, 256, 511, 512,
		1023, 1024, 2047, 2048, 4000, 8000
	};
	int count = (int)(sizeof(sizes) / sizeof(sizes[0]));

	for (int i = 0; i < count; i++) {
		void *p = kmalloc(sizes[i]);
		KTEST_NOT_NULL(p, "kmalloc returns non-null");

		memset(p, 0xCC, sizes[i]);
		size_t usable = kmalloc_usable_size(p);
		KTEST_GE(usable, sizes[i], "usable >= requested");

		kfree(p);
	}
}
