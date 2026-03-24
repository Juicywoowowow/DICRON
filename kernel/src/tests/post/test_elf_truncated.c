#include "../ktest.h"
#include <dicron/elf.h>

KTEST_REGISTER(ktest_elf_truncated, "ELF: truncated", KTEST_CAT_POST)
static void ktest_elf_truncated(void)
{
	KTEST_BEGIN("elf: truncated input");

	uint8_t buf[4] = {0x7f, 'E', 'L', 'F'};

	/* Too small for ehdr */
	int rc = elf64_validate(buf, 4);
	KTEST_EQ(rc, ELF_ERR_TRUNC, "truncated: 4 bytes rejected");

	rc = elf64_validate(buf, 0);
	KTEST_EQ(rc, ELF_ERR_TRUNC, "truncated: 0 bytes rejected");

	rc = elf64_validate(buf, sizeof(struct elf64_ehdr) - 1);
	KTEST_EQ(rc, ELF_ERR_TRUNC, "truncated: ehdr-1 bytes rejected");
}
