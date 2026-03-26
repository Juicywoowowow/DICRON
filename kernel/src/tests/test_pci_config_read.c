#include "ktest.h"
#include "drivers/pci/pci.h"

KTEST_REGISTER(test_pci_config_read, "PCI: config space reads", KTEST_CAT_BOOT)
static void test_pci_config_read(void)
{
	KTEST_BEGIN("PCI: config space reads");

	if (pci_device_count() == 0)
		return;

	struct pci_device *d = pci_get_device(0);

	/* Read vendor via 16-bit should match stored value */
	uint16_t vid = pci_config_read16(d->bus, d->dev, d->func,
					 PCI_VENDOR_ID);
	KTEST_EQ(vid, d->vendor_id, "read16 vendor matches");

	/* Read device via 16-bit */
	uint16_t did = pci_config_read16(d->bus, d->dev, d->func,
					 PCI_DEVICE_ID);
	KTEST_EQ(did, d->device_id, "read16 device matches");

	/* Read class via 8-bit */
	uint8_t cls = pci_config_read8(d->bus, d->dev, d->func,
				       PCI_CLASS);
	KTEST_EQ(cls, d->class_code, "read8 class matches");

	/* Read full 32-bit word at offset 0: vendor | device << 16 */
	uint32_t word = pci_config_read32(d->bus, d->dev, d->func, 0);
	KTEST_EQ((uint16_t)(word & 0xFFFF), d->vendor_id,
		 "read32 lo = vendor");
	KTEST_EQ((uint16_t)(word >> 16), d->device_id,
		 "read32 hi = device");

	/* Non-existent device returns 0xFFFF */
	uint16_t ghost = pci_config_read16(0, 31, 7, PCI_VENDOR_ID);
	KTEST_EQ(ghost, 0xFFFF, "empty slot = 0xFFFF");
}
