#include "ktest.h"
#include "../drivers/pci/ahci/ahci.h"
#include "../drivers/pci/pci.h"

KTEST_REGISTER(ktest_ahci_pci_class, "ahci: PCI class 01:06 found", KTEST_CAT_BOOT)
static void ktest_ahci_pci_class(void)
{
	KTEST_BEGIN("ahci: PCI class 01:06 found");
	struct pci_device *pci = pci_find_class(PCI_CLASS_STORAGE,
						PCI_SUBCLASS_SATA);
	if (!pci) {
		KTEST_TRUE(1, "ahci: no SATA controller (skip)");
		return;
	}
	KTEST_EQ((int)pci->class_code, (int)PCI_CLASS_STORAGE,
		 "ahci: class is storage");
	KTEST_EQ((int)pci->subclass, (int)PCI_SUBCLASS_SATA,
		 "ahci: subclass is SATA");
}
