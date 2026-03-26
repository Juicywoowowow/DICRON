Device Drivers Subsystem
========================

1. Overview
-----------

DICRON includes various device drivers for hardware interaction:

  Storage:   ATA/IDE, Ramdisk
  Input:     PS/2 Keyboard
  Timing:    PIT (Programmable Interval Timer), RTC
  Bus:       PCI
  Output:    Serial, Framebuffer Console

2. Driver Architecture
---------------------

Drivers follow a consistent pattern:
  - Initialization function
  - Public API functions
  - Private data structures

3. Driver Registration
---------------------

The driver registry (src/drivers/registry.c) provides:
  - Central driver list
  - Driver initialization ordering

4. Block Devices
----------------

Block device interface (blkdev.h):
  - read(block, buf): Read block
  - write(block, buf): Write block
  - block_size: Size of each block
  - total_blocks: Total number of blocks

5. Character Devices
-------------------

Character devices use file_operations:
  - read: Input from device
  - write: Output to device
  - ioctl: Device-specific control

6. Optional Drivers
------------------

Many drivers are optional via Kconfig:
  - CONFIG_PCI: Enable PCI bus driver
  - CONFIG_ATA: Enable ATA disk driver
  - CONFIG_PS2_KBD: Enable PS/2 keyboard
  - CONFIG_PIT: Enable PIT timer
  - CONFIG_FRAMEBUFFER: Enable framebuffer console

7. PCI Enumeration
------------------

The PCI driver enumerates devices at boot:
  - Scans all buses, devices, functions
  - Builds device table for drivers
  - Drivers can look up devices by class

8. Interrupt Handling
--------------------

Device interrupts use IDT:
  - Hardware IRQ handlers registered
  - Device-specific processing in handler
