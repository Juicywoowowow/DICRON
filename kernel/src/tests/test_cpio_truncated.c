#include "ktest.h"
#include <dicron/cpio.h>
#include "lib/string.h"

static void test_cpio_truncated(void)
{
	/* Archive smaller than header */
	char tiny[16];
	memset(tiny, 0, sizeof(tiny));
	memcpy(tiny, "070701", 6);

	struct cpio_entry entry;
	size_t ret = cpio_parse_entry(tiny, sizeof(tiny), 0, &entry);
	KTEST_EQ((long long)ret, 0, "truncated archive returns 0");

	/* Offset past end */
	char buf[CPIO_HEADER_SIZE];
	memset(buf, '0', sizeof(buf));
	ret = cpio_parse_entry(buf, sizeof(buf), 200, &entry);
	KTEST_EQ((long long)ret, 0, "offset past end returns 0");
}

KTEST_REGISTER(test_cpio_truncated, "CPIO: truncated archive handled", KTEST_CAT_BOOT)
