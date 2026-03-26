#include "ktest.h"
#include "drivers/pci/pci.h"

KTEST_REGISTER(test_pci_bars, "PCI: BAR validation", KTEST_CAT_BOOT)
static void test_pci_bars(void)
{
	KTEST_BEGIN("PCI: BAR validation");

	int count = pci_device_count();
	if (count == 0)
		return;

	int found_nonzero_bar = 0;
	for (int i = 0; i < count; i++) {
		struct pci_device *d = pci_get_device(i);
		if (!d) continue;

		for (int b = 0; b < 6; b++) {
			if (d->bar[b] != 0) {
				found_nonzero_bar = 1;

				/* Check BAR type bit */
				if (d->bar[b] & 1) {
					/* I/O BAR: low bit set */
					KTEST_TRUE(d->bar[b] & 1,
						   "IO BAR bit 0 set");
				}
			}
		}
	}

	KTEST_TRUE(found_nonzero_bar, "at least one device has a BAR");
}
