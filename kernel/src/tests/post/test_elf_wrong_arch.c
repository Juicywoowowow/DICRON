#include "../ktest.h"
#include <dicron/elf.h>
#include "lib/string.h"

KTEST_REGISTER(ktest_elf_wrong_arch, "ELF: wrong arch", KTEST_CAT_POST)
static void ktest_elf_wrong_arch(void)
{
	KTEST_BEGIN("elf: wrong architecture");

	uint8_t buf[256];
	memset(buf, 0, sizeof(buf));
	struct elf64_ehdr *ehdr = (struct elf64_ehdr *)buf;

	/* Valid magic but 32-bit class */
	ehdr->e_ident[0] = 0x7f;
	ehdr->e_ident[1] = 'E';
	ehdr->e_ident[2] = 'L';
	ehdr->e_ident[3] = 'F';
	ehdr->e_ident[EI_CLASS] = 1; /* ELFCLASS32 */
	ehdr->e_ident[EI_DATA] = ELFDATA2LSB;
	ehdr->e_ident[EI_VERSION] = EV_CURRENT;
	ehdr->e_type = ET_EXEC;
	ehdr->e_machine = EM_X86_64;
	ehdr->e_version = EV_CURRENT;

	int rc = elf64_validate(buf, sizeof(buf));
	KTEST_EQ(rc, ELF_ERR_CLASS, "wrong_arch: 32-bit class rejected");

	/* 64-bit but ARM machine */
	ehdr->e_ident[EI_CLASS] = ELFCLASS64;
	ehdr->e_machine = 40; /* EM_ARM */
	ehdr->e_entry = 0x400000;
	ehdr->e_phoff = sizeof(struct elf64_ehdr);
	ehdr->e_phentsize = sizeof(struct elf64_phdr);
	ehdr->e_phnum = 1;

	/* Need a valid phdr too for it to get past phdr checks */
	struct elf64_phdr *phdr = (struct elf64_phdr *)(buf + ehdr->e_phoff);
	phdr->p_type = PT_LOAD;
	phdr->p_vaddr = 0x400000;
	phdr->p_filesz = 64;
	phdr->p_memsz = 64;

	rc = elf64_validate(buf, sizeof(buf));
	KTEST_EQ(rc, ELF_ERR_MACHINE, "wrong_arch: ARM machine rejected");
}
