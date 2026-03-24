#include "../ktest.h"
#include <dicron/elf.h>
#include "lib/string.h"

KTEST_REGISTER(ktest_elf_bad_magic, "ELF: bad magic", KTEST_CAT_POST)
static void ktest_elf_bad_magic(void)
{
	KTEST_BEGIN("elf: bad magic");

	uint8_t buf[256];
	memset(buf, 0, sizeof(buf));

	/* All zeros */
	int rc = elf64_validate(buf, sizeof(buf));
	KTEST_EQ(rc, ELF_ERR_MAGIC, "bad_magic: all zeros");

	/* Correct first 3, wrong 4th */
	buf[0] = 0x7f; buf[1] = 'E'; buf[2] = 'L'; buf[3] = 'X';
	rc = elf64_validate(buf, sizeof(buf));
	KTEST_EQ(rc, ELF_ERR_MAGIC, "bad_magic: ELX rejected");

	/* MZ header (PE executable) */
	memset(buf, 0, sizeof(buf));
	buf[0] = 'M'; buf[1] = 'Z';
	rc = elf64_validate(buf, sizeof(buf));
	KTEST_EQ(rc, ELF_ERR_MAGIC, "bad_magic: MZ rejected");
}
