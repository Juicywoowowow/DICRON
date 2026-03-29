#include "ktest.h"
#include "../drivers/pci/ahci/ahci.h"

KTEST_REGISTER(ktest_ahci_port_count, "ahci: port count valid", KTEST_CAT_BOOT)
static void ktest_ahci_port_count(void)
{
	KTEST_BEGIN("ahci: port count valid");
	int count = ahci_port_count();
	KTEST_GE(count, 0, "ahci: port count >= 0");
	KTEST(count <= AHCI_MAX_PORTS, "ahci: port count <= 32");
}

KTEST_REGISTER(ktest_ahci_port_regs, "ahci: port regs non-null when active", KTEST_CAT_BOOT)
static void ktest_ahci_port_regs(void)
{
	KTEST_BEGIN("ahci: port regs non-null when active");
	if (ahci_port_count() == 0) {
		KTEST_TRUE(1, "ahci: no ports (skip)");
		return;
	}
	struct ahci_port *p = ahci_get_port(0);
	KTEST_NOT_NULL(p, "ahci: port[0] is non-null");
	KTEST_NOT_NULL(p->regs, "ahci: port[0] regs present");
	KTEST_NOT_NULL(p->cmd_list, "ahci: port[0] cmd_list allocated");
	KTEST_NOT_NULL(p->fis_base, "ahci: port[0] fis_base allocated");
	KTEST_NOT_NULL(p->cmd_tbl, "ahci: port[0] cmd_tbl allocated");
}
