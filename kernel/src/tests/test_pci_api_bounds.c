#include "ktest.h"
#include "drivers/pci/pci.h"

KTEST_REGISTER(test_pci_api_bounds, "PCI: API bounds", KTEST_CAT_BOOT)
static void test_pci_api_bounds(void)
{
	KTEST_BEGIN("PCI: API bounds");

	KTEST_GE(pci_device_count(), 0, "count >= 0");
	KTEST_LT(pci_device_count(), PCI_MAX_DEVICES + 1,
		  "count <= max");

	KTEST_NULL(pci_get_device(-1), "negative index");
	KTEST_NULL(pci_get_device(PCI_MAX_DEVICES), "max index");
	KTEST_NULL(pci_get_device(9999), "huge index");

	/* Non-existent vendor/device */
	KTEST_NULL(pci_find_device(0xDEAD, 0xBEEF),
		   "bogus vendor:device");
}
