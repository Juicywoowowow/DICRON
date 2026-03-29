#include "sata.h"
#include "drivers/pci/ahci/ahci.h"
#include <generated/autoconf.h>

#ifdef CONFIG_SATA

int sata_detect_ports(void);

void sata_init(void)
{
	ahci_init();

	if (ahci_port_count() == 0)
		return;

	sata_detect_ports();
}

#endif /* CONFIG_SATA */
