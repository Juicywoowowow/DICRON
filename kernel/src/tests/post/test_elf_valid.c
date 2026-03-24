#include "../ktest.h"
#include <dicron/elf.h>
#include "lib/string.h"

/* Build a minimal valid ELF64 executable in memory */
static void build_valid_elf(uint8_t *buf, size_t *out_size)
{
	memset(buf, 0, 256);
	struct elf64_ehdr *ehdr = (struct elf64_ehdr *)buf;

	ehdr->e_ident[EI_MAG0] = ELFMAG0;
	ehdr->e_ident[EI_MAG1] = ELFMAG1;
	ehdr->e_ident[EI_MAG2] = ELFMAG2;
	ehdr->e_ident[EI_MAG3] = ELFMAG3;
	ehdr->e_ident[EI_CLASS] = ELFCLASS64;
	ehdr->e_ident[EI_DATA] = ELFDATA2LSB;
	ehdr->e_ident[EI_VERSION] = EV_CURRENT;
	ehdr->e_type = ET_EXEC;
	ehdr->e_machine = EM_X86_64;
	ehdr->e_version = EV_CURRENT;
	ehdr->e_entry = 0x400000;
	ehdr->e_phoff = sizeof(struct elf64_ehdr);
	ehdr->e_phentsize = sizeof(struct elf64_phdr);
	ehdr->e_phnum = 1;
	ehdr->e_ehsize = sizeof(struct elf64_ehdr);

	struct elf64_phdr *phdr = (struct elf64_phdr *)(buf + ehdr->e_phoff);
	phdr->p_type = PT_LOAD;
	phdr->p_flags = PF_R | PF_X;
	phdr->p_offset = 0;
	phdr->p_vaddr = 0x400000;
	phdr->p_filesz = sizeof(struct elf64_ehdr) + sizeof(struct elf64_phdr);
	phdr->p_memsz = phdr->p_filesz;
	phdr->p_align = 0x1000;

	*out_size = sizeof(struct elf64_ehdr) + sizeof(struct elf64_phdr);
}

KTEST_REGISTER(ktest_elf_valid, "ELF: valid", KTEST_CAT_POST)
static void ktest_elf_valid(void)
{
	KTEST_BEGIN("elf: valid binary");

	uint8_t buf[256];
	size_t sz;
	build_valid_elf(buf, &sz);

	int rc = elf64_validate(buf, sz);
	KTEST_EQ(rc, ELF_OK, "elf_valid: minimal valid ELF passes");
}
