#include "../ktest.h"
#include <dicron/elf.h>
#include "lib/string.h"

KTEST_REGISTER(ktest_elf_overlapping_segments, "ELF: overlapping segments", KTEST_CAT_POST)
static void ktest_elf_overlapping_segments(void)
{
	KTEST_BEGIN("elf: overlapping segments");

	uint8_t buf[512];
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
	ehdr->e_entry = 0x400000;
	ehdr->e_phoff = sizeof(struct elf64_ehdr);
	ehdr->e_phentsize = sizeof(struct elf64_phdr);
	ehdr->e_phnum = 2;
	ehdr->e_ehsize = sizeof(struct elf64_ehdr);

	struct elf64_phdr *phdr0 = (struct elf64_phdr *)(buf + ehdr->e_phoff);
	struct elf64_phdr *phdr1 = (struct elf64_phdr *)((uint8_t *)phdr0 + sizeof(struct elf64_phdr));

	/* Segment 0: 0x400000 - 0x401000 */
	phdr0->p_type = PT_LOAD;
	phdr0->p_flags = PF_R | PF_X;
	phdr0->p_offset = 0;
	phdr0->p_vaddr = 0x400000;
	phdr0->p_filesz = 64;
	phdr0->p_memsz = 0x1000;

	/* Segment 1: 0x400800 - overlaps segment 0 */
	phdr1->p_type = PT_LOAD;
	phdr1->p_flags = PF_R | PF_W;
	phdr1->p_offset = 0;
	phdr1->p_vaddr = 0x400800;
	phdr1->p_filesz = 64;
	phdr1->p_memsz = 0x1000;

	size_t total = sizeof(struct elf64_ehdr) + 2 * sizeof(struct elf64_phdr);
	int rc = elf64_validate(buf, total);
	KTEST_EQ(rc, ELF_ERR_OVERLAP, "overlap: overlapping segments rejected");
}
