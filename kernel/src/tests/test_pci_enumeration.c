#include "ktest.h"
#include "drivers/pci/pci.h"

KTEST_REGISTER(test_pci_enumeration, "PCI: device enumeration", KTEST_CAT_BOOT)
static void test_pci_enumeration(void)
{
	KTEST_BEGIN("PCI: device enumeration");

	int count = pci_device_count();
	KTEST_GT(count, 0, "at least 1 PCI device");

	for (int i = 0; i < count; i++) {
		struct pci_device *d = pci_get_device(i);
		KTEST_NOT_NULL(d, "device not NULL");
		if (!d) continue;

		KTEST_NE(d->vendor_id, 0xFFFF, "valid vendor id");
		KTEST_NE(d->vendor_id, 0x0000, "nonzero vendor id");
		KTEST_TRUE(d->bus < PCI_MAX_BUS, "bus in range");
		KTEST_TRUE(d->dev < PCI_MAX_DEV, "dev in range");
		KTEST_TRUE(d->func < PCI_MAX_FUNC, "func in range");
	}
}
