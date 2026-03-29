#include "ahci.h"
#include "drivers/pci/pci.h"
#include "lib/string.h"
#include "mm/pmm.h"
#include "mm/vmm.h"
#include <dicron/errno.h>
#include <dicron/log.h>
#include <generated/autoconf.h>

#ifdef CONFIG_AHCI

#define AHCI_MAX_ACTIVE_PORTS 8
#define AHCI_POLL_TIMEOUT     200000

static struct ahci_hba_regs *hba;
static struct ahci_port ports[AHCI_MAX_ACTIVE_PORTS];
static int nports;

static void port_stop(struct ahci_port_regs *pr)
{
	pr->cmd &= ~AHCI_CMD_ST;
	for (int i = 0; i < AHCI_POLL_TIMEOUT; i++) {
		if (!(pr->cmd & AHCI_CMD_CR))
			break;
	}
	pr->cmd &= ~AHCI_CMD_FRE;
	for (int i = 0; i < AHCI_POLL_TIMEOUT; i++) {
		if (!(pr->cmd & AHCI_CMD_FR))
			break;
	}
}

static void port_free_dma(void *clb, void *fb, void *ct)
{
	pmm_free_page(clb);
	pmm_free_page(fb);
	pmm_free_page(ct);
}

static int ahci_init_port(struct ahci_port_regs *pr, int port_num)
{
	if (nports >= AHCI_MAX_ACTIVE_PORTS)
		return -ENOMEM;

	port_stop(pr);

	void *clb = pmm_alloc_page();
	if (!clb)
		return -ENOMEM;
	memset(clb, 0, PAGE_SIZE);

	void *fb = pmm_alloc_page();
	if (!fb) {
		pmm_free_page(clb);
		return -ENOMEM;
	}
	memset(fb, 0, PAGE_SIZE);

	void *ct = pmm_alloc_page();
	if (!ct) {
		pmm_free_page(clb);
		pmm_free_page(fb);
		return -ENOMEM;
	}
	memset(ct, 0, PAGE_SIZE);

	uint64_t hhdm = vmm_get_hhdm();
	uint64_t clb_phys = ahci_virt_to_phys(clb, hhdm);
	uint64_t fb_phys = ahci_virt_to_phys(fb, hhdm);
	uint64_t ct_phys = ahci_virt_to_phys(ct, hhdm);

	pr->clb  = (uint32_t)(clb_phys);
	pr->clbu = (uint32_t)(clb_phys >> 32);
	pr->fb   = (uint32_t)(fb_phys);
	pr->fbu  = (uint32_t)(fb_phys >> 32);

	struct ahci_cmd_hdr *hdr = (struct ahci_cmd_hdr *)clb;
	hdr[0].ctba  = (uint32_t)(ct_phys);
	hdr[0].ctbau = (uint32_t)(ct_phys >> 32);

	pr->serr = 0xFFFFFFFF;
	pr->is = 0xFFFFFFFF;
	pr->cmd |= AHCI_CMD_FRE;

	/* COMRESET */
	pr->sctl = (pr->sctl & ~0xFU) | 1;
	for (volatile int d = 0; d < 10000; d++)
		;
	pr->sctl &= ~0xFU;

	/* Wait for PHY link */
	int detected = 0;
	for (int i = 0; i < AHCI_POLL_TIMEOUT; i++) {
		if ((pr->ssts & AHCI_SSTS_DET_MASK) == AHCI_SSTS_DET_PRESENT) {
			detected = 1;
			break;
		}
	}
	if (!detected) {
		pr->cmd &= ~AHCI_CMD_FRE;
		port_free_dma(clb, fb, ct);
		return -ENODEV;
	}

	pr->serr = 0xFFFFFFFF;

	for (int i = 0; i < AHCI_POLL_TIMEOUT; i++) {
		if (!(pr->tfd & (AHCI_TFD_BSY | AHCI_TFD_DRQ)))
			break;
	}

	if (pr->sig != AHCI_SIG_SATA) {
		port_stop(pr);
		port_free_dma(clb, fb, ct);
		return -ENODEV;
	}

	pr->cmd |= AHCI_CMD_ST;

	struct ahci_port *p = &ports[nports];
	p->port_num = port_num;
	p->regs = pr;
	p->cmd_list = (struct ahci_cmd_hdr *)clb;
	p->fis_base = fb;
	p->cmd_tbl = (struct ahci_cmd_tbl *)ct;

	klog(KLOG_DEBUG, "ahci: port %d active\n", port_num);
	nports++;
	return 0;
}

static int ahci_init_hba(struct pci_device *pci)
{
	uint32_t bar5 = pci->bar[5];
	if (bar5 & 1) {
		klog(KLOG_WARN, "ahci: BAR5 is I/O, expected MMIO\n");
		return -ENODEV;
	}

	uint64_t bar5_phys = (uint64_t)(bar5 & ~0xFU);
	uint64_t hhdm = vmm_get_hhdm();
	uint64_t bar5_virt = bar5_phys + hhdm;

	uint64_t phys_page = bar5_phys & ~0xFFFULL;
	uint64_t virt_page = bar5_virt & ~0xFFFULL;
	for (uint64_t off = 0; off < 0x3000; off += 0x1000)
		vmm_map_page(virt_page + off, phys_page + off,
			     VMM_PRESENT | VMM_WRITE);

	hba = (struct ahci_hba_regs *)(uintptr_t)bar5_virt;

	uint16_t cmd = pci_config_read16(pci->bus, pci->dev,
					 pci->func, PCI_COMMAND);
	cmd = (uint16_t)(cmd | 0x06);
	pci_config_write16(pci->bus, pci->dev, pci->func,
			   PCI_COMMAND, cmd);

	hba->ghc |= AHCI_GHC_AE;

	if (hba->cap2 & (1U << 0)) {
		hba->bohc |= AHCI_BOHC_OOS;
		for (int i = 0; i < AHCI_POLL_TIMEOUT; i++) {
			if (!(hba->bohc & AHCI_BOHC_BOS))
				break;
		}
	}

	uint32_t pi = hba->pi;
	klog(KLOG_DEBUG, "ahci: HBA v%d.%d, %d ports\n",
	     (int)(hba->vs >> 16), (int)(hba->vs & 0xFFFF),
	     (int)(hba->cap & 0x1F) + 1);

	for (int i = 0; i < AHCI_MAX_PORTS; i++) {
		if (pi & (1U << i))
			ahci_init_port(&hba->ports[i], i);
	}

	return 0;
}

void ahci_init(void)
{
#ifndef CONFIG_PCI
	return;
#else
	struct pci_device *pci = pci_find_class(PCI_CLASS_STORAGE,
						PCI_SUBCLASS_SATA);
	if (!pci)
		return;

	ahci_init_hba(pci);

	if (nports > 0)
		klog(KLOG_INFO, "ahci: %d SATA port(s) active\n", nports);
#endif
}

int ahci_port_count(void)
{
	return nports;
}

struct ahci_port *ahci_get_port(int index)
{
	if (index < 0 || index >= nports)
		return NULL;
	return &ports[index];
}

#endif /* CONFIG_AHCI */
