DICRON Kernel Documentation
============================

1. Overview
-----------

DICRON is a x86_64 operating system kernel designed for educational purposes
and embedded applications. It implements a Unix-like userspace environment
compatible with the musl libc library.

2. System Architecture
-----------------------

The kernel is organized into the following major subsystems:

  - arch/x86_64:     CPU-specific code (GDT, IDT, TSS, syscall entry)
  - mm:              Memory management (PMM, VMM, slab allocator, heap)
  - sched:           Multilevel Feedback Queue scheduler
  - syscall:         System call dispatch and handlers
  - proc:            Process creation and ELF loading
  - fs:              Virtual File System, RAMFS, devfs, ext2
  - drivers:         Device drivers (ATA, PCI, PS/2, serial, timer)
  - console:         Framebuffer console output
  - io:              Kernel logging and I/O utilities

3. Boot Process
---------------

DICRON uses the Limine bootloader protocol for bootstrapping. The kernel
is loaded by Limine and receives:

  - Memory map (limine_memmap_request)
  - Framebuffer information (limine_framebuffer_request)
  - Higher half direct map offset (limine_hhdm_request)
  - Initrd modules (limine_module_request)

The entry point is kmain() in src/main.c, which performs the following
initialization sequence:

  1. Enable SSE floating-point instructions
  2. Initialize GDT and TSS
  3. Set up Interrupt Descriptor Table (IDT)
  4. Initialize Physical Memory Manager (PMM)
  5. Initialize Virtual Memory Manager (VMM)
  6. Initialize kernel heap
  7. Initialize console (framebuffer or serial)
  8. Initialize serial port
  9. Initialize PS/2 keyboard driver
  10. Initialize Programmable Interval Timer (PIT)
  11. Initialize Real-Time Clock (RTC)
  12. Initialize PCI bus (if configured)
  13. Initialize ATA/IDE disks (if configured)
  14. Set up system call interface
  15. Initialize VFS and filesystems
  16. Run boot tests (if configured)
  17. Initialize scheduler
  18. Extract initrd (cpio archive)
  19. Load and execute /init ELF binary

4. Memory Layout
----------------

The kernel uses a higher half direct mapping (HHDM) approach:

  0xFFFF800000000000+: Kernel space (identity-mapped to physical)
  0x0000800000000000+: User space (per-process)

Each user process has its own page table (PML4) created by VMM.
The kernel maintains a separate page table for its own use.

5. System Call Interface
------------------------

System calls follow the Linux x86_64 ABI:

  - syscall instruction with number in RAX
  - Arguments in RDI, RSI, RDX, R10, R8, R9
  - Return value in RAX

The kernel implements a subset of Linux syscalls for musl compatibility.

6. Scheduling
------------

DICRON uses a Multilevel Feedback Queue (MLFQ) scheduler with 4 queues.
Each queue has a different timeslice:

  - Queue 0: 2 ms (highest priority)
  - Queue 1: 4 ms
  - Queue 2: 8 ms
  - Queue 3: 16 ms (lowest priority)

Priority boosting occurs every 500 ms to prevent starvation.

7. Filesystem
-------------

The kernel implements a Unix-like VFS with multiple backends:

  - ramfs:   In-memory filesystem for initrd
  - devfs:   Device files (/dev/null, /dev/console, etc.)
  - ext2:    Secondary storage filesystem

8. Building
-----------

The kernel is built using GNU Make. See GNUmakefile for build targets.

9. Testing
----------

The kernel includes an extensive test suite in src/tests/:

  - Boot tests:    Run before scheduler initialization
  - Post tests:    Run after scheduler is active

Tests can be enabled via Kconfig (CONFIG_TESTS).


10. Kconfig
-----------

The kernel uses Kconfig for configuration:

  - make menuconfig: Interactive configuration
  - make defconfig: Default configuration
  - make savedefconfig: Save current configuration

Configuration options control:
  - Kernel features
  - Driver availability
  - Test execution
  - Debug output

11. Debugging
-------------

DICRON supports multiple debugging methods:

  - Serial port logging
  - Kernel log (klog)
  - Test output
  - QEMU serial console
  - GDB debugging (via QEMU)

  12. Issues
  ---------
  Documentation might be incomplete or inaccurate

    - Most doc files or subfolders are incomplete
    - Some files are outdated
    - Some files are missing
    - Some files are not updated (None for now, will be a problem later however.)