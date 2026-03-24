#ifndef _DICRON_ELF_H
#define _DICRON_ELF_H

#include <stddef.h>
#include <stdint.h>

/* ELF magic */
#define ELFMAG0 0x7f
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'

/* e_ident indices */
#define EI_MAG0 0
#define EI_MAG1 1
#define EI_MAG2 2
#define EI_MAG3 3
#define EI_CLASS 4
#define EI_DATA 5
#define EI_VERSION 6
#define EI_NIDENT 16

/* EI_CLASS */
#define ELFCLASS64 2

/* EI_DATA */
#define ELFDATA2LSB 1

/* e_type */
#define ET_EXEC 2

/* e_machine */
#define EM_X86_64 62

/* e_version */
#define EV_CURRENT 1

/* p_type */
#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3
#define PT_NOTE 4
#define PT_PHDR 6

/* p_flags */
#define PF_X 0x1
#define PF_W 0x2
#define PF_R 0x4

struct elf64_ehdr {
  uint8_t e_ident[EI_NIDENT];
  uint16_t e_type;
  uint16_t e_machine;
  uint32_t e_version;
  uint64_t e_entry;
  uint64_t e_phoff;
  uint64_t e_shoff;
  uint32_t e_flags;
  uint16_t e_ehsize;
  uint16_t e_phentsize;
  uint16_t e_phnum;
  uint16_t e_shentsize;
  uint16_t e_shnum;
  uint16_t e_shstrndx;
};

struct elf64_phdr {
  uint32_t p_type;
  uint32_t p_flags;
  uint64_t p_offset;
  uint64_t p_vaddr;
  uint64_t p_paddr;
  uint64_t p_filesz;
  uint64_t p_memsz;
  uint64_t p_align;
};

/* Return codes */
#define ELF_OK 0
#define ELF_ERR_MAGIC -1
#define ELF_ERR_CLASS -2
#define ELF_ERR_MACHINE -3
#define ELF_ERR_TYPE -4
#define ELF_ERR_VERSION -5
#define ELF_ERR_PHDR -6
#define ELF_ERR_NOENTRY -7
#define ELF_ERR_OVERLAP -8
#define ELF_ERR_TRUNC -9
#define ELF_ERR_NOMEM -10

/*
 * Validate an ELF64 header.
 * Returns ELF_OK on success, negative error code on failure.
 */
int elf64_validate(const void *data, size_t size);

/*
 * Load an ELF64 binary into the CURRENT address space.
 * Maps PT_LOAD segments at their virtual addresses with VMM_USER.
 * Returns the entry point on success, 0 on failure.
 */
uint64_t elf64_load(const void *data, size_t size);

/*
 * Load ELF into a specific address space. Sets *brk_out to end of segments.
 */
uint64_t elf64_load_into(const void *data, size_t size,
                         uint64_t pml4_phys, uint64_t *brk_out);

#endif
