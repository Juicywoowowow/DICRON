Boot Process
============

1. Overview
-----------

DICRON uses the Limine bootloader protocol for bootstrapping. The
bootloader provides memory information, framebuffer, and the initial
ramdisk before control is transferred to the kernel.

2. Limine Requests
------------------

The kernel requests the following from Limine:

  - Base Revision: Ensures compatible Limine version
  - Framebuffer: Graphics output device
  - Memory Map: Available physical memory regions
  - HHDM: Higher Half Direct Map offset
  - Modules: Initrd (cpio archive) and other modules

3. Boot Flow
------------

  [Bootloader] -> [Limine Protocol] -> [Kernel Entry]

  1. Limine loads kernel to specified address
  2. Limine provides response structures
  3. Kernel reads response structures
  4. Kernel initializes subsystems
  5. Kernel extracts initrd
  6. Kernel loads and executes /init

4. Initialization Sequence (kmain)
-----------------------------------

The kernel main function performs initialization in this order:

  1. Enable SSE (floating point)
  2. Validate Limine base revision
  3. Initialize GDT
  4. Initialize TSS
  5. Initialize IDT (interrupt handlers)
  6. Validate memory map and HHDM response
  7. Initialize PMM (physical memory)
  8. Enable SSE again (for user programs)
  9. Initialize VMM (virtual memory)
  10. Initialize kernel heap
  11. Initialize console (framebuffer or skip)
  12. Initialize serial port
  13. Initialize keyboard (if enabled)
  14. Initialize PIT timer (if enabled)
  15. Initialize RTC
  16. Initialize PCI bus (if enabled)
  17. Initialize ATA driver (if enabled)
  18. Initialize syscall interface
  19. Initialize VFS
  20. Initialize devfs
  21. Initialize ramfs
  22. Log PMM statistics
  23. Run boot tests (if enabled)
  24. Initialize scheduler
  25. Run post-boot tests (if enabled)
  26. Extract initrd from module
  27. Find and load /init ELF binary
  28. Create init process
  29. Start scheduler loop

5. Limine Response Structures
-----------------------------

  struct limine_framebuffer_response
      - framebuffer_count
      - framebuffers[] (address, width, height, bpp, pitch)

  struct limine_memmap_response
      - entry_count
      - entries[] (base, length, type)

  struct limine_hhdm_response
      - offset (for physical to virtual mapping)

  struct limine_module_response
      - module_count
      - modules[] (address, size, path)

6. Memory Types
---------------

Limine memory map types:
  - 0: Usable RAM
  - 1: Reserved
  - 2: ACPI reclaimable
  - 3: ACPI NVS
  - 4: Bad memory
  - 5: Reserved (bootloader)
  - 6: Reserved (kernel)
  - 7: Device memory

The PMM only uses regions marked as type 0 (Usable).

7. Higher Half Direct Mapping
-----------------------------

The HHDM provides a direct mapping:
  physical_addr + hhdm_offset = virtual_addr

For example, if offset = 0xFFFF800000000000:
  0x00000000 + 0xFFFF800000000000 = 0xFFFF800000000000

This allows the kernel to access physical memory using virtual
addresses without manipulating page tables.
