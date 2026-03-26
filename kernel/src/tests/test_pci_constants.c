#include "ktest.h"
#include "drivers/pci/pci.h"

KTEST_REGISTER(test_pci_constants, "PCI: port/offset constants", KTEST_CAT_BOOT)
static void test_pci_constants(void)
{
	KTEST_BEGIN("PCI: port/offset constants");

	KTEST_EQ(PCI_CONFIG_ADDR, 0xCF8, "config addr port");
	KTEST_EQ(PCI_CONFIG_DATA, 0xCFC, "config data port");

	KTEST_EQ(PCI_VENDOR_ID, 0x00, "vendor id offset");
	KTEST_EQ(PCI_DEVICE_ID, 0x02, "device id offset");
	KTEST_EQ(PCI_COMMAND, 0x04, "command offset");
	KTEST_EQ(PCI_CLASS, 0x0B, "class offset");
	KTEST_EQ(PCI_SUBCLASS, 0x0A, "subclass offset");
	KTEST_EQ(PCI_HEADER_TYPE, 0x0E, "header type offset");
	KTEST_EQ(PCI_BAR0, 0x10, "BAR0 offset");
	KTEST_EQ(PCI_BAR5, 0x24, "BAR5 offset");
	KTEST_EQ(PCI_IRQ_LINE, 0x3C, "IRQ line offset");

	/* BAR offsets should be 4 apart */
	KTEST_EQ(PCI_BAR1 - PCI_BAR0, 4, "BAR spacing");
	KTEST_EQ(PCI_BAR2 - PCI_BAR1, 4, "BAR spacing 2");
}
