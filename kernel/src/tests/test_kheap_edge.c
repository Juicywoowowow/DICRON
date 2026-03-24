#include "ktest.h"
#include "mm/kheap.h"

KTEST_REGISTER(ktest_kheap_edge, "kmalloc edge cases", KTEST_CAT_BOOT)
static void ktest_kheap_edge(void)
{
	KTEST_BEGIN("kmalloc edge cases");

	KTEST_NULL(kmalloc(0), "kmalloc(0) returns NULL");

	/* 1 GB — exceeds MAX_ORDER, must return NULL */
	void *huge = kmalloc(1024ULL * 1024ULL * 1024ULL);
	KTEST_NULL(huge, "kmalloc(1GB) returns NULL");

	/* kfree(NULL) must not crash */
	kfree((void *)0);
	KTEST_TRUE(1, "kfree(NULL) no crash");

	/* kzalloc returns zeroed memory */
	unsigned char *z = kzalloc(128);
	KTEST_NOT_NULL(z, "kzalloc(128) non-null");
	int ok = 1;
	for (int i = 0; i < 128; i++)
		if (z[i] != 0) ok = 0;
	KTEST_TRUE(ok, "kzalloc memory is zeroed");
	kfree(z);
}
