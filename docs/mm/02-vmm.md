Virtual Memory Manager (VMM)
============================

1. Overview
-----------

The Virtual Memory Manager (VMM) provides virtual address space
management using x86_64 page tables. It supports:
  - Creating per-process address spaces
  - Mapping virtual pages to physical pages
  - Translating virtual addresses to physical
  - User/kernel address space separation

2. Page Table Structure
-----------------------

The VMM uses the x86_64 four-level page table structure:

  PML4 (Page Map Level 4) -> 512 entries
    PDPT (Page Directory Pointer Table) -> 512 entries each
      PD (Page Directory) -> 512 entries each
        PT (Page Table) -> 512 entries each

Each entry is 8 bytes, covering 2 MiB (PT) to 512 GiB (PML4).

The kernel uses a separate PML4 with a higher half direct mapping
that identity-maps physical memory at virtual addresses above
0xFFFF800000000000.

3. Page Table Flags
-------------------

The following flags are used for page table entries:

  VMM_PRESENT (bit 0): Page is present in memory
  VMM_WRITE   (bit 1): Page is writable
  VMM_USER    (bit 2): Page is accessible from userspace
  VMM_NX      (bit 63): No-execute (disables code execution)

4. API
------

Initialization:
  void vmm_init(uint64_t hhdm_offset)
      - Initialize VMM with HHDM offset

Per-process address spaces:
  uint64_t vmm_create_user_pml4(void)
      - Create a new empty page table for a process
      - Returns physical address of PML4

  void vmm_switch_to(uint64_t pml4_phys)
      - Switch to a different address space
      - Loads the specified PML4 into CR3

  uint64_t vmm_get_cr3(void)
      - Get physical address of current PML4

  uint64_t vmm_get_hhdm(void)
      - Get HHDM offset for virtual-to-physical conversion

Mapping:
  int vmm_map_page(uint64_t virt, uint64_t phys, uint64_t flags)
      - Map a page in the kernel address space
      - Returns 0 on success, -1 on failure

  int vmm_map_page_in(uint64_t pml4_phys, uint64_t virt, uint64_t phys, uint64_t flags)
      - Map a page in a specific address space
      - Returns 0 on success, -1 on failure

  void vmm_unmap_page(uint64_t virt)
      - Remove a mapping from kernel address space

  void vmm_unmap_page_in(uint64_t pml4_phys, uint64_t virt)
      - Remove a mapping from a specific address space

Translation:
  uint64_t vmm_virt_to_phys(uint64_t virt)
      - Translate virtual address to physical
      - Returns physical address, or 0 on failure

  uint64_t vmm_virt_to_phys_in(uint64_t pml4_phys, uint64_t virt)
      - Translate virtual address in a specific address space

5. Address Space Layout
-----------------------

User space (per-process):
  0x0000000000000000 - 0x00007FFFFFFFFFFF (128 TiB)
  - Shared library area
  - Program code and data
  - Heap (grows up)
  - Stack (grows down)

Kernel space:
  0xFFFF800000000000+
  - Direct mapping of all physical memory
  - Kernel code and data
  - vmalloc region

6. Page Fault Handling
----------------------

When a page fault occurs (invalid page access), the CPU triggers an
exception. The kernel's IDT handler for page faults records the event
but currently does not implement demand paging.

7. Usage in Kernel
------------------

The VMM is used by:
  - Process creation: Creating user address spaces
  - ELF loading: Mapping program segments
  - sys_mmap: Allocating user memory
  - Kernel heap: Large allocations may use vmm_map_page

8. Security
-----------

The VMM enforces the following security measures:
  - User pages are marked with VMM_USER flag
  - Kernel pages are not accessible from userspace
  - NX bit is set on pages that should not be executable
