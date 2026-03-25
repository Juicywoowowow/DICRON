#include "ktest.h"
#include <dicron/cpio.h>
#include "lib/string.h"

/* Build a minimal valid newc cpio entry in a buffer.
 * Returns total bytes written. */
static size_t build_cpio_entry(char *buf, size_t bufsz,
			       const char *name, uint32_t mode,
			       const char *data, uint32_t datalen)
{
	if (bufsz < CPIO_HEADER_SIZE) return 0;
	uint32_t namesize = (uint32_t)strlen(name) + 1; /* includes NUL */

	/* Fill header with hex ASCII fields */
	memset(buf, '0', CPIO_HEADER_SIZE);
	memcpy(buf, "070701", 6);

	/* Write mode at offset 14 */
	char tmp[9];
	for (int i = 7; i >= 0; i--) {
		uint32_t nibble = mode & 0xF;
		tmp[i] = (char)(nibble < 10 ? '0' + nibble : 'a' + nibble - 10);
		mode >>= 4;
	}
	memcpy(buf + 14, tmp, 8);

	/* Write filesize at offset 54 */
	uint32_t fs = datalen;
	for (int i = 7; i >= 0; i--) {
		uint32_t nibble = fs & 0xF;
		tmp[i] = (char)(nibble < 10 ? '0' + nibble : 'a' + nibble - 10);
		fs >>= 4;
	}
	memcpy(buf + 54, tmp, 8);

	/* Write namesize at offset 94 */
	uint32_t ns = namesize;
	for (int i = 7; i >= 0; i--) {
		uint32_t nibble = ns & 0xF;
		tmp[i] = (char)(nibble < 10 ? '0' + nibble : 'a' + nibble - 10);
		ns >>= 4;
	}
	memcpy(buf + 94, tmp, 8);

	/* Name follows header, aligned to 4 */
	size_t name_off = CPIO_HEADER_SIZE;
	memcpy(buf + name_off, name, namesize);
	size_t data_off = (name_off + namesize + 3) & ~(size_t)3;

	/* Data follows name, aligned to 4 */
	if (datalen > 0 && data)
		memcpy(buf + data_off, data, datalen);

	return (data_off + datalen + 3) & ~(size_t)3;
}

static void test_cpio_parse_entry(void)
{
	char buf[512];
	memset(buf, 0, sizeof(buf));

	size_t written = build_cpio_entry(buf, sizeof(buf),
					  "hello.txt", 0100644, "Hi!", 3);
	KTEST_GT((long long)written, 0, "built test cpio entry");

	struct cpio_entry entry;
	size_t consumed = cpio_parse_entry(buf, written, 0, &entry);
	KTEST_GT((long long)consumed, 0, "parse_entry returns >0");
	KTEST_EQ((long long)entry.filesize, 3, "filesize is 3");
	KTEST_EQ((long long)entry.mode, 0100644, "mode is 0100644");
	KTEST_NOT_NULL(entry.data, "data pointer is set");
	KTEST_TRUE(memcmp(entry.data, "Hi!", 3) == 0, "data content matches");
}

KTEST_REGISTER(test_cpio_parse_entry, "CPIO: parse valid entry", KTEST_CAT_BOOT)
