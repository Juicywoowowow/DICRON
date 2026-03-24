#include "../ktest.h"
#include <dicron/elf.h>
#include "lib/string.h"

KTEST_REGISTER(ktest_elf_no_entry, "ELF: no entry", KTEST_CAT_POST)
static void ktest_elf_no_entry(void)
{
	KTEST_BEGIN("elf: no entry point");

	uint8_t buf[256];
	memset(buf, 0, sizeof(buf));
	struct elf64_ehdr *ehdr = (struct elf64_ehdr *)buf;

	ehdr->e_ident[0] = 0x7f;
	ehdr->e_ident[1] = 'E';
	ehdr->e_ident[2] = 'L';
	ehdr->e_ident[3] = 'F';
	ehdr->e_ident[EI_CLASS] = ELFCLASS64;
	ehdr->e_ident[EI_DATA] = ELFDATA2LSB;
	ehdr->e_ident[EI_VERSION] = EV_CURRENT;
	ehdr->e_type = ET_EXEC;
	ehdr->e_machine = EM_X86_64;
	ehdr->e_version = EV_CURRENT;
	ehdr->e_entry = 0;  /* no entry */
	ehdr->e_phoff = sizeof(struct elf64_ehdr);
	ehdr->e_phentsize = sizeof(struct elf64_phdr);
	ehdr->e_phnum = 1;

	struct elf64_phdr *phdr = (struct elf64_phdr *)(buf + ehdr->e_phoff);
	phdr->p_type = PT_LOAD;
	phdr->p_vaddr = 0x400000;
	phdr->p_filesz = 64;
	phdr->p_memsz = 64;

	size_t sz = sizeof(struct elf64_ehdr) + sizeof(struct elf64_phdr);
	int rc = elf64_validate(buf, sz);
	KTEST_EQ(rc, ELF_ERR_NOENTRY, "no_entry: zero entry point rejected");
}
