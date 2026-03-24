#include "ktest.h"
#include "lib/string.h"

KTEST_REGISTER(ktest_string, "string library", KTEST_CAT_BOOT)
static void ktest_string(void)
{
	KTEST_BEGIN("string library");

	/* strlen */
	KTEST_EQ(strlen(""), 0, "strlen empty");
	KTEST_EQ(strlen("hello"), 5, "strlen hello");

	/* memset */
	char buf[32];
	memset(buf, 0xAA, sizeof(buf));
	int ok = 1;
	for (int i = 0; i < 32; i++)
		if ((unsigned char)buf[i] != 0xAA) ok = 0;
	KTEST_TRUE(ok, "memset fill");

	/* memcpy */
	char src[] = "DICRON";
	char dst[8];
	memcpy(dst, src, 7);
	KTEST_EQ(memcmp(dst, "DICRON", 7), 0, "memcpy content");

	/* memmove overlap forward */
	char overlap[] = "ABCDEF";
	memmove(overlap + 2, overlap, 4);
	KTEST_EQ(memcmp(overlap + 2, "ABCD", 4), 0, "memmove overlap fwd");

	/* memmove overlap backward */
	char overlap2[] = "ABCDEF";
	memmove(overlap2, overlap2 + 2, 4);
	KTEST_EQ(memcmp(overlap2, "CDEF", 4), 0, "memmove overlap bwd");

	/* strncmp */
	KTEST_EQ(strncmp("abc", "abd", 2), 0, "strncmp partial match");
	KTEST_TRUE(strncmp("abc", "abd", 3) < 0, "strncmp mismatch");

	/* memcmp */
	KTEST_EQ(memcmp("AAA", "AAA", 3), 0, "memcmp equal");
	KTEST_TRUE(memcmp("AAA", "AAB", 3) < 0, "memcmp less");
	KTEST_TRUE(memcmp("AAB", "AAA", 3) > 0, "memcmp greater");

	/* memset zero */
	memset(buf, 0, sizeof(buf));
	ok = 1;
	for (int i = 0; i < 32; i++)
		if (buf[i] != 0) ok = 0;
	KTEST_TRUE(ok, "memset zero");
}
