PCI Bus Driver
==============

1. Overview
-----------

The PCI (Peripheral Component Interconnect) driver provides enumeration
and configuration access to PCI devices. It scans the PCI bus to discover
connected devices and provides access to their configuration space.

2. Configuration Space
-----------------------

PCI uses两层 configuration space:
  - Type 1: Per-bus, per-device, per-function addressing
  - Accessed through I/O ports 0xCF8 (address) and 0xCFC (data)

Configuration Address Register Format:
  [31]    - Enable bit (1 = access enabled)
  [30:24] - Reserved
  [23:16] - Bus number (8 bits)
  [15:11] - Device number (5 bits)
  [10:8]  - Function number (3 bits)
  [7:2]   - Register offset (6 bits)
  [1:0]   - Reserved (00)

3. I/O Ports
------------

  0xCF8 - PCI Configuration Address
  0xCFC - PCI Configuration Data

4. Configuration Registers
---------------------------

  0x00 - Vendor ID
  0x02 - Device ID
  0x04 - Command
  0x06 - Status
  0x08 - Revision ID
  0x09 - Prog IF
  0x0A - Subclass
  0x0B - Class Code
  0x0E - Header Type
  0x10-0x24 - BAR0-BAR5 (Base Address Registers)
  0x3C - IRQ Line

5. Class Codes
--------------

  0x01 - Storage
  0x02 - Network
  0x03 - Display
  0x06 - Bridge
  0x0C - Serial

Storage Subclasses:
  0x01 - IDE
  0x06 - SATA
  0x08 - NVMe

6. API
-----

  void pci_init(void)
      - Initialize PCI, enumerate all devices

Configuration Access:
  uint32_t pci_config_read32(bus, dev, func, offset)
  uint16_t pci_config_read16(bus, dev, func, offset)
  uint8_t  pci_config_read8(bus, dev, func, offset)

  void pci_config_write32(bus, dev, func, offset, val)
  void pci_config_write16(bus, dev, func, offset, val)

Device Discovery:
  int pci_device_count(void)
      - Get number of discovered PCI devices

  struct pci_device *pci_get_device(int index)
      - Get device by index

  struct pci_device *pci_find_device(uint16_t vendor, uint16_t device)
      - Find device by vendor/device ID

  struct pci_device *pci_find_class(uint8_t class_code, uint8_t subclass)
      - Find device by class code

7. Device Structure
-------------------

  struct pci_device {
      uint8_t  bus, dev, func;
      uint16_t vendor_id, device_id;
      uint8_t  class_code, subclass, prog_if;
      uint8_t  header_type;
      uint8_t  irq_line;
      uint32_t bar[6];
  };

8. Enumeration
--------------

The PCI driver scans all 256 buses, 32 devices per bus, 8 functions per
device. For each combination, it reads the vendor ID - a value of 0xFFFF
indicates no device present.
