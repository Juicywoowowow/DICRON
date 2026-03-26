DICRON Kernel
=============

WHAT IS DICRON?

  DICRON is a 64-bit (x86_64) Unix-like operating system kernel built from
  scratch. It is designed with clean abstractions, focusing on robust process
  scheduling, memory management, and compatibility with the Linux x86_64 ABI
  to natively run static C applications compiled against the musl libc library.

HARDWARE SUPPORT

  Currently, DICRON strictly targets the x86_64 architecture. Key hardware features
  include:
  - PCI enumeration and configuration.
  - ATA PIO disk devices with MBR partition table parsing.
  - PS/2 Keyboard input logic.
  - Basic PC serial (COM1) for remote debugging.
  - High-precision PIT execution and RTC integration.
  - Framebuffer display via the Limine bootloader interface.
  - Native SSE/SSE2 instruction set utilization for application contexts.
BOOTLOADER (LIMINE)

  DICRON relies exclusively on the Limine Bootloader Protocol for bootstrapping.
  Limine is responsible for preparing the physical memory map, setting up Higher Half
  Direct Mapping (HHDM), assigning base kernel stacks, passing framebuffer layout 
  structures, and extracting the initial ramdisk (initrd) archive directly into memory
  before transferring execution to `kmain()`. Limine streamlines pre-boot execution, 
  avoiding the typical constraints and complexities of Legacy BIOS or raw UEFI routines.

COMPILATION AND DEPLOYMENT

  1. System Toolchain
     A standard GNU toolchain is required alongside NASM, xorriso, and QEMU.
     For Debian/Ubuntu base targets:
     $ sudo apt-get install gcc nasm make xorriso mtools qemu-system-x86_64 git

  2. Downloading Limine
     You must download the Limine bootloader binaries via git clone prior to building.
     DICRON is strictly compatible only with Limine version 6.x and above. Versions
     5.x and below are entirely unsupported and will cause boot failures.
     $ git clone https://github.com/limine-bootloader/limine.git Limine --branch v6.x-branch-binary --depth=1
     $ make -C Limine

  3. Building musl libc
     DICRON applications utilize stable musl routines locally cross-compiled.
     Execute the following across the primary project tree:
     $ git clone git://git.musl-libc.org/musl
     $ cd musl
     $ ./configure --prefix=$PWD/../musl-install --disable-shared
     $ make -j$(nproc)
     $ make install
     $ cd ..

  4. Configuring the Kernel
     The system employs a rigid Kconfig-like structure utilizing GNU make semantics.
     $ make menuconfig

  5. Building the OS Image
     Compilation will dynamically resolve local dependencies, inject Limine boot 
     structures, and produce an ISO functional directly inside QEMU.
     $ make run

DEBUGGING AND TESTS
  
  During standard boot cycles, DICRON performs rigorous integrated test validations
  against Page bounds, Slab exhaustions, and Scheduler synchronization mechanisms. 
  Extensive logging isolates states securely to the QEMU serial output via `klog`.
  System crashes or user-mode execution violations prompt full register traces aligned
  directly against Limine's hardware topology reports to reliably track debugging contexts.

