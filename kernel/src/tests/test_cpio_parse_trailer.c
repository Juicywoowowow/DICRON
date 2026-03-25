#include "ktest.h"
#include <dicron/cpio.h>
#include "lib/string.h"

static void test_cpio_parse_trailer(void)
{
	/* Build a TRAILER!!! entry — should return 0 */
	char buf[256];
	memset(buf, '0', sizeof(buf));
	memcpy(buf, "070701", 6);

	/* namesize = 11 (TRAILER!!! + NUL) = 0x0000000b */
	memcpy(buf + 94, "0000000b", 8);
	/* filesize = 0 */

	/* Name at offset 110 */
	memcpy(buf + CPIO_HEADER_SIZE, "TRAILER!!!", 11);

	struct cpio_entry entry;
	size_t ret = cpio_parse_entry(buf, sizeof(buf), 0, &entry);
	KTEST_EQ((long long)ret, 0, "TRAILER returns 0 (stop)");
}

KTEST_REGISTER(test_cpio_parse_trailer, "CPIO: trailer stops parsing", KTEST_CAT_BOOT)
