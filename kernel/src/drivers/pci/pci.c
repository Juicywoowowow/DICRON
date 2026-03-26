#include "pci.h"
#include "arch/x86_64/io.h"
#include <dicron/log.h>
#include "lib/string.h"

static struct pci_device devices[PCI_MAX_DEVICES];
static int num_devices;

static uint32_t pci_addr(uint8_t bus, uint8_t dev,
			 uint8_t func, uint8_t offset)
{
	return (1U << 31) |
	       ((uint32_t)bus << 16) |
	       ((uint32_t)(dev & 0x1F) << 11) |
	       ((uint32_t)(func & 0x07) << 8) |
	       ((uint32_t)offset & 0xFC);
}

uint32_t pci_config_read32(uint8_t bus, uint8_t dev,
			   uint8_t func, uint8_t offset)
{
	outl(PCI_CONFIG_ADDR, pci_addr(bus, dev, func, offset));
	return inl(PCI_CONFIG_DATA);
}

uint16_t pci_config_read16(uint8_t bus, uint8_t dev,
			   uint8_t func, uint8_t offset)
{
	uint32_t val = pci_config_read32(bus, dev, func,
					 (uint8_t)(offset & 0xFC));
	return (uint16_t)(val >> ((offset & 2) * 8));
}

uint8_t pci_config_read8(uint8_t bus, uint8_t dev,
			 uint8_t func, uint8_t offset)
{
	uint32_t val = pci_config_read32(bus, dev, func,
					 (uint8_t)(offset & 0xFC));
	return (uint8_t)(val >> ((offset & 3) * 8));
}

void pci_config_write32(uint8_t bus, uint8_t dev,
			uint8_t func, uint8_t offset, uint32_t val)
{
	outl(PCI_CONFIG_ADDR, pci_addr(bus, dev, func, offset));
	outl(PCI_CONFIG_DATA, val);
}

void pci_config_write16(uint8_t bus, uint8_t dev,
			uint8_t func, uint8_t offset, uint16_t val)
{
	uint32_t old = pci_config_read32(bus, dev, func,
					 (uint8_t)(offset & 0xFC));
	int shift = (offset & 2) * 8;
	old &= ~((uint32_t)0xFFFF << shift);
	old |= (uint32_t)val << shift;
	pci_config_write32(bus, dev, func,
			   (uint8_t)(offset & 0xFC), old);
}

static void pci_probe_func(uint8_t bus, uint8_t dev, uint8_t func)
{
	uint16_t vendor = pci_config_read16(bus, dev, func, PCI_VENDOR_ID);
	if (vendor == 0xFFFF)
		return;

	if (num_devices >= PCI_MAX_DEVICES)
		return;

	struct pci_device *d = &devices[num_devices];
	d->bus = bus;
	d->dev = dev;
	d->func = func;
	d->vendor_id = vendor;
	d->device_id = pci_config_read16(bus, dev, func, PCI_DEVICE_ID);
	d->class_code = pci_config_read8(bus, dev, func, PCI_CLASS);
	d->subclass = pci_config_read8(bus, dev, func, PCI_SUBCLASS);
	d->prog_if = pci_config_read8(bus, dev, func, PCI_PROG_IF);
	d->header_type = pci_config_read8(bus, dev, func, PCI_HEADER_TYPE);
	d->irq_line = pci_config_read8(bus, dev, func, PCI_IRQ_LINE);

	for (int i = 0; i < 6; i++)
		d->bar[i] = pci_config_read32(bus, dev, func,
			(uint8_t)(PCI_BAR0 + i * 4));

	klog(KLOG_INFO, "pci %02x:%02x.%x: %04x:%04x class=%02x:%02x\n",
	     bus, dev, func, d->vendor_id, d->device_id,
	     d->class_code, d->subclass);

	num_devices++;
}

static void pci_probe_device(uint8_t bus, uint8_t dev)
{
	uint16_t vendor = pci_config_read16(bus, dev, 0, PCI_VENDOR_ID);
	if (vendor == 0xFFFF)
		return;

	pci_probe_func(bus, dev, 0);

	/* Multi-function device? */
	uint8_t header = pci_config_read8(bus, dev, 0, PCI_HEADER_TYPE);
	if (header & 0x80) {
		for (uint8_t func = 1; func < PCI_MAX_FUNC; func++)
			pci_probe_func(bus, dev, func);
	}
}

void pci_init(void)
{
	num_devices = 0;

	for (int bus = 0; bus < PCI_MAX_BUS; bus++) {
		for (int dev = 0; dev < PCI_MAX_DEV; dev++)
			pci_probe_device((uint8_t)bus, (uint8_t)dev);
	}

	klog(KLOG_INFO, "pci: %d device(s) found\n", num_devices);
}

int pci_device_count(void)
{
	return num_devices;
}

struct pci_device *pci_get_device(int index)
{
	if (index < 0 || index >= num_devices)
		return NULL;
	return &devices[index];
}

struct pci_device *pci_find_device(uint16_t vendor, uint16_t device)
{
	for (int i = 0; i < num_devices; i++) {
		if (devices[i].vendor_id == vendor &&
		    devices[i].device_id == device)
			return &devices[i];
	}
	return NULL;
}

struct pci_device *pci_find_class(uint8_t class_code, uint8_t subclass)
{
	for (int i = 0; i < num_devices; i++) {
		if (devices[i].class_code == class_code &&
		    devices[i].subclass == subclass)
			return &devices[i];
	}
	return NULL;
}
