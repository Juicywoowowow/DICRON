#ifndef _DICRON_DRIVERS_PCI_H
#define _DICRON_DRIVERS_PCI_H

#include <stdint.h>

#define PCI_CONFIG_ADDR 0xCF8
#define PCI_CONFIG_DATA 0xCFC

#define PCI_MAX_BUS     256
#define PCI_MAX_DEV     32
#define PCI_MAX_FUNC    8

/* Config space offsets */
#define PCI_VENDOR_ID    0x00
#define PCI_DEVICE_ID    0x02
#define PCI_COMMAND      0x04
#define PCI_STATUS       0x06
#define PCI_REVISION     0x08
#define PCI_PROG_IF      0x09
#define PCI_SUBCLASS     0x0A
#define PCI_CLASS        0x0B
#define PCI_HEADER_TYPE  0x0E
#define PCI_BAR0         0x10
#define PCI_BAR1         0x14
#define PCI_BAR2         0x18
#define PCI_BAR3         0x1C
#define PCI_BAR4         0x20
#define PCI_BAR5         0x24
#define PCI_IRQ_LINE     0x3C

/* Class codes */
#define PCI_CLASS_STORAGE    0x01
#define PCI_CLASS_NETWORK    0x02
#define PCI_CLASS_DISPLAY    0x03
#define PCI_CLASS_BRIDGE     0x06
#define PCI_CLASS_SERIAL     0x0C

/* Storage subclasses */
#define PCI_SUBCLASS_IDE     0x01
#define PCI_SUBCLASS_SATA    0x06
#define PCI_SUBCLASS_NVME    0x08

/* Max tracked devices */
#define PCI_MAX_DEVICES  64

struct pci_device {
	uint8_t  bus;
	uint8_t  dev;
	uint8_t  func;
	uint16_t vendor_id;
	uint16_t device_id;
	uint8_t  class_code;
	uint8_t  subclass;
	uint8_t  prog_if;
	uint8_t  header_type;
	uint8_t  irq_line;
	uint32_t bar[6];
};

void pci_init(void);

uint32_t pci_config_read32(uint8_t bus, uint8_t dev,
			   uint8_t func, uint8_t offset);
uint16_t pci_config_read16(uint8_t bus, uint8_t dev,
			   uint8_t func, uint8_t offset);
uint8_t  pci_config_read8(uint8_t bus, uint8_t dev,
			  uint8_t func, uint8_t offset);

void pci_config_write32(uint8_t bus, uint8_t dev,
			uint8_t func, uint8_t offset, uint32_t val);
void pci_config_write16(uint8_t bus, uint8_t dev,
			uint8_t func, uint8_t offset, uint16_t val);

int pci_device_count(void);
struct pci_device *pci_get_device(int index);
struct pci_device *pci_find_device(uint16_t vendor, uint16_t device);
struct pci_device *pci_find_class(uint8_t class_code, uint8_t subclass);

#endif
