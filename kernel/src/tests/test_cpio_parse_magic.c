#include "ktest.h"
#include <dicron/cpio.h>
#include "lib/string.h"

static void test_cpio_parse_magic(void)
{
	/* Invalid magic should fail */
	char bad[CPIO_HEADER_SIZE];
	memset(bad, '0', sizeof(bad));
	memcpy(bad, "BADHDR", 6);

	struct cpio_entry entry;
	size_t ret = cpio_parse_entry(bad, sizeof(bad), 0, &entry);
	KTEST_EQ((long long)ret, 0, "bad magic returns 0");
}

KTEST_REGISTER(test_cpio_parse_magic, "CPIO: reject bad magic", KTEST_CAT_BOOT)
