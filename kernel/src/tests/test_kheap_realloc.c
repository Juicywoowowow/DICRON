#include "ktest.h"
#include "mm/kheap.h"
#include "lib/string.h"

KTEST_REGISTER(ktest_kheap_realloc, "krealloc", KTEST_CAT_BOOT)
static void ktest_kheap_realloc(void)
{
	KTEST_BEGIN("krealloc");

	/* grow: data preserved */
	void *p = kmalloc(32);
	memset(p, 'A', 32);
	void *p2 = krealloc(p, 256);
	KTEST_NOT_NULL(p2, "krealloc grow returns non-null");

	unsigned char *b = p2;
	int ok = 1;
	for (int i = 0; i < 32; i++)
		if (b[i] != 'A') ok = 0;
	KTEST_TRUE(ok, "krealloc preserves data on grow");
	kfree(p2);

	/* krealloc(NULL, n) == kmalloc(n) */
	void *p3 = krealloc((void *)0, 64);
	KTEST_NOT_NULL(p3, "krealloc(NULL, 64) acts as kmalloc");
	kfree(p3);

	/* krealloc(p, 0) == kfree(p) */
	void *p4 = kmalloc(64);
	void *p5 = krealloc(p4, 0);
	KTEST_NULL(p5, "krealloc(p, 0) acts as kfree");

	/* cross-cache promotion: 16 -> 1024 */
	void *p6 = kmalloc(16);
	memset(p6, 'X', 16);
	void *p7 = krealloc(p6, 1024);
	KTEST_NOT_NULL(p7, "cross-cache krealloc non-null");
	KTEST_NE((uint64_t)p6, (uint64_t)p7, "cross-cache moves pointer");
	b = p7;
	ok = 1;
	for (int i = 0; i < 16; i++)
		if (b[i] != 'X') ok = 0;
	KTEST_TRUE(ok, "cross-cache data preserved");
	kfree(p7);
}
