ELF Format Support
===================

1. Overview
-----------

DICRON supports loading ELF64 (Executable and Linkable Format, 64-bit)
binaries as user processes. The ELF loader validates and loads programs
into user address spaces.

2. ELF Structure
----------------

  +------------------+
  | ELF Header       |  - Magic, type, machine, entry, program header table
  +------------------+
  | Program Headers  |  - Segments to load (PT_LOAD)
  +------------------+
  | Section Headers  |  - Named sections (not used at runtime)
  +------------------+
  | Program Data     |  - Actual code and data
  +------------------+

3. ELF Header (struct elf64_ehdr)
---------------------------------

  e_ident[16]   - Magic and identification
  e_type        - ET_EXEC for executable
  e_machine     - EM_X86_64
  e_version     - EV_CURRENT
  e_entry       - Entry point virtual address
  e_phoff       - Program header offset
  e_shoff       - Section header offset
  e_flags       - Flags
  e_ehsize      - ELF header size
  e_phentsize   - Program header entry size
  e_phnum       - Program header entry count
  e_shentsize   - Section header entry size
  e_shnum       - Section header entry count
  e_shstrndx    - Section name string table index

4. ELF Identification
--------------------

  e_ident[EI_MAG0..EI_MAG3] = 0x7F, 'E', 'L', 'F'
  e_ident[EI_CLASS] = ELFCLASS64
  e_ident[EI_DATA] = ELFDATA2LSB (little-endian)

5. Program Header (struct elf64_phdr)
-------------------------------------

  p_type     - PT_LOAD for loadable segments
  p_offset   - Offset in file
  p_vaddr    - Virtual address
  p_paddr    - Physical address (ignored)
  p_filesz   - Size in file
  p_memsz    - Size in memory (may be larger than filesz for BSS)
  p_flags    - PF_X (execute), PF_W (write), PF_R (read)
  p_align    - Alignment

6. Validation
-------------

The kernel validates ELF files for:
  - Correct magic number
  - 64-bit class
  - Little-endian encoding
  - Executable type (ET_EXEC)
  - x86_64 machine type
  - Valid entry point (non-zero)
  - Valid program headers
  - No overlapping segments
  - Addresses in user space range

7. Loading Process
------------------

  1. Validate ELF header and program headers
  2. Create new address space (PML4)
  3. For each PT_LOAD segment:
     - Calculate number of pages needed
     - Map pages with appropriate flags
     - Copy file data into pages
     - Zero any BSS region (memsz > filesz)
  4. Set up user stack
  5. Return entry point address

8. Error Codes
--------------

  ELF_OK             0  - Success
  ELF_ERR_MAGIC     -1  - Invalid magic
  ELF_ERR_CLASS     -2  - Wrong class (not 64-bit)
  ELF_ERR_TYPE      -3  - Wrong type
  ELF_ERR_MACHINE   -4  - Wrong machine
  ELF_ERR_VERSION  -5  - Invalid version
  ELF_ERR_NOENTRY  -6  - No entry point
  ELF_ERR_PHDR     -7  - Invalid program headers
  ELF_ERR_TRUNC    -8  - Truncated file
  ELF_ERR_OVERLAP  -9  - Overlapping segments

9. Address Restrictions
-----------------------

DICRON restricts ELF addresses:
  - Must be below 0x0000800000000000
  - Must not overlap between segments
  - Entry point must be non-zero
