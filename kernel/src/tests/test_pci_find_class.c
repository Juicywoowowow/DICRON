#include "ktest.h"
#include "drivers/pci/pci.h"

KTEST_REGISTER(test_pci_find_class, "PCI: find by class", KTEST_CAT_BOOT)
static void test_pci_find_class(void)
{
	KTEST_BEGIN("PCI: find by class");

	/* QEMU q35 always has a host bridge (class 06:00) */
	struct pci_device *bridge = pci_find_class(PCI_CLASS_BRIDGE, 0x00);
	KTEST_NOT_NULL(bridge, "found host bridge");
	if (bridge) {
		KTEST_EQ(bridge->class_code, PCI_CLASS_BRIDGE,
			 "bridge class");
		KTEST_EQ(bridge->subclass, 0x00, "host bridge subclass");
	}

	/* Bogus class should not match */
	KTEST_NULL(pci_find_class(0xFF, 0xFF), "bogus class not found");
}
