#include "sata.h"
#include "drivers/pci/ahci/ahci.h"
#include <dicron/errno.h>
#include <generated/autoconf.h>

#ifdef CONFIG_SATA

int sata_read(struct sata_device *dev, uint64_t lba,
	      uint32_t count, void *buf)
{
	if (!dev || !buf || !dev->present)
		return -EINVAL;

	struct ahci_port *port = ahci_get_port(dev->port_index);
	if (!port)
		return -ENODEV;

	return ahci_cmd_read_dma(port, lba, count, buf);
}

int sata_write(struct sata_device *dev, uint64_t lba,
	       uint32_t count, const void *buf)
{
	if (!dev || !buf || !dev->present)
		return -EINVAL;

	struct ahci_port *port = ahci_get_port(dev->port_index);
	if (!port)
		return -ENODEV;

	return ahci_cmd_write_dma(port, lba, count, buf);
}

#endif /* CONFIG_SATA */
