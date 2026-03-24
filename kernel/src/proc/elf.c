#include <dicron/elf.h>
#include <dicron/log.h>
#include "mm/vmm.h"
#include "mm/pmm.h"
#include "lib/string.h"

/*
 * elf.c — ELF64 validation and loading.
 */

int elf64_validate(const void *data, size_t size)
{
	if (size < sizeof(struct elf64_ehdr))
		return ELF_ERR_TRUNC;

	const struct elf64_ehdr *ehdr = (const struct elf64_ehdr *)data;

	if (ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
	    ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
	    ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
	    ehdr->e_ident[EI_MAG3] != ELFMAG3)
		return ELF_ERR_MAGIC;

	if (ehdr->e_ident[EI_CLASS] != ELFCLASS64)
		return ELF_ERR_CLASS;
	if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB)
		return ELF_ERR_CLASS;

	if (ehdr->e_type != ET_EXEC)
		return ELF_ERR_TYPE;
	if (ehdr->e_machine != EM_X86_64)
		return ELF_ERR_MACHINE;
	if (ehdr->e_version != EV_CURRENT)
		return ELF_ERR_VERSION;
	if (ehdr->e_entry == 0)
		return ELF_ERR_NOENTRY;

	if (ehdr->e_phoff == 0 || ehdr->e_phnum == 0)
		return ELF_ERR_PHDR;

	uint64_t ph_end = ehdr->e_phoff + (uint64_t)ehdr->e_phnum * ehdr->e_phentsize;
	if (ph_end > size)
		return ELF_ERR_TRUNC;
	if (ehdr->e_phentsize < sizeof(struct elf64_phdr))
		return ELF_ERR_PHDR;

	const uint8_t *raw = (const uint8_t *)data;
	uint64_t prev_end = 0;

	for (uint16_t i = 0; i < ehdr->e_phnum; i++) {
		const struct elf64_phdr *phdr =
			(const struct elf64_phdr *)(raw + ehdr->e_phoff +
						    (uint64_t)i * ehdr->e_phentsize);
		if (phdr->p_type != PT_LOAD)
			continue;
		if (phdr->p_offset + phdr->p_filesz > size)
			return ELF_ERR_TRUNC;
		if (phdr->p_memsz < phdr->p_filesz)
			return ELF_ERR_PHDR;
		if (phdr->p_vaddr >= 0x0000800000000000ULL)
			return ELF_ERR_PHDR;
		uint64_t seg_end = phdr->p_vaddr + phdr->p_memsz;
		if (seg_end > 0x0000800000000000ULL)
			return ELF_ERR_PHDR;
		if (phdr->p_vaddr < prev_end)
			return ELF_ERR_OVERLAP;
		prev_end = seg_end;
	}

	return ELF_OK;
}

/*
 * Load ELF segments into a specific address space (pml4_phys).
 * Returns entry point on success, 0 on failure.
 * Also sets *brk_out to the end of the last loaded segment (for sys_brk).
 */
uint64_t elf64_load_into(const void *data, size_t size,
			 uint64_t pml4_phys, uint64_t *brk_out)
{
	int rc = elf64_validate(data, size);
	if (rc != ELF_OK) {
		klog(KLOG_ERR, "elf64_load: validation failed (%d)\n", rc);
		return 0;
	}

	const struct elf64_ehdr *ehdr = (const struct elf64_ehdr *)data;
	const uint8_t *raw = (const uint8_t *)data;
	uint64_t hhdm = vmm_get_hhdm();
	uint64_t highest = 0;

	for (uint16_t i = 0; i < ehdr->e_phnum; i++) {
		const struct elf64_phdr *phdr =
			(const struct elf64_phdr *)(raw + ehdr->e_phoff +
						    (uint64_t)i * ehdr->e_phentsize);
		if (phdr->p_type != PT_LOAD)
			continue;

		uint64_t vaddr_start = phdr->p_vaddr & ~0xFFFULL;
		uint64_t vaddr_end = (phdr->p_vaddr + phdr->p_memsz + 0xFFF) & ~0xFFFULL;
		size_t num_pages = (size_t)((vaddr_end - vaddr_start) / 4096);

		uint64_t vmflags = VMM_PRESENT | VMM_USER;
		if (phdr->p_flags & PF_W)
			vmflags |= VMM_WRITE;
		if (!(phdr->p_flags & PF_X))
			vmflags |= VMM_NX;

		for (size_t p = 0; p < num_pages; p++) {
			uint64_t va = vaddr_start + p * 4096;
			void *page = pmm_alloc_page();
			if (!page) {
				klog(KLOG_ERR, "elf64_load: OOM mapping segment\n");
				return 0;
			}
			memset(page, 0, 4096);
			uint64_t phys = (uint64_t)page - hhdm;

			if (vmm_map_page_in(pml4_phys, va, phys, vmflags) < 0) {
				pmm_free_page(page);
				return 0;
			}

			/* Copy file data */
			uint64_t seg_file_start = phdr->p_vaddr;
			uint64_t seg_file_end = phdr->p_vaddr + phdr->p_filesz;

			if (va + 4096 > seg_file_start && va < seg_file_end) {
				uint64_t cs = va > seg_file_start ? va : seg_file_start;
				uint64_t ce = (va + 4096) < seg_file_end ? (va + 4096) : seg_file_end;
				size_t copy_len = (size_t)(ce - cs);
				size_t file_off = (size_t)(phdr->p_offset + (cs - phdr->p_vaddr));
				size_t page_off = (size_t)(cs - va);
				memcpy((uint8_t *)page + page_off, raw + file_off, copy_len);
			}
		}

		if (vaddr_end > highest)
			highest = vaddr_end;
	}

	if (brk_out)
		*brk_out = highest;

	return ehdr->e_entry;
}

/* Wrapper for validation-only tests */
uint64_t elf64_load(const void *data, size_t size)
{
	return elf64_load_into(data, size, vmm_get_cr3(), NULL);
}
