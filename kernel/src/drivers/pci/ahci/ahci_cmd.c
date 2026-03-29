#include "ahci.h"
#include "mm/vmm.h"
#include "lib/string.h"
#include <dicron/errno.h>
#include <dicron/log.h>
#include <generated/autoconf.h>

#ifdef CONFIG_AHCI

#define AHCI_CMD_TIMEOUT	5000000
#define AHCI_CFL_H2D		5
#define AHCI_CMD_WRITE_BIT	(1U << 6)

static int ahci_submit_slot0(struct ahci_port *port)
{
	struct ahci_port_regs *pr = port->regs;

	pr->is = 0xFFFFFFFF;
	pr->ci = 1;

	for (int i = 0; i < AHCI_CMD_TIMEOUT; i++) {
		if (!(pr->ci & 1))
			break;
		if (pr->is & AHCI_PxIS_TFES) {
			klog(KLOG_ERR, "ahci: port %d TFE (TFD=0x%x)\n",
			     port->port_num, pr->tfd);
			return -EIO;
		}
	}

	if (pr->ci & 1) {
		klog(KLOG_ERR, "ahci: port %d timeout\n", port->port_num);
		return -EIO;
	}

	if (pr->tfd & AHCI_TFD_ERR)
		return -EIO;

	return 0;
}

static void build_h2d_fis(struct ahci_cmd_tbl *tbl, uint8_t command,
			   uint64_t lba, uint16_t count)
{
	memset(tbl->cfis, 0, sizeof(tbl->cfis));

	struct fis_reg_h2d *fis = (struct fis_reg_h2d *)tbl->cfis;
	fis->type = FIS_TYPE_REG_H2D;
	fis->pmport_c = 0x80;
	fis->command = command;
	fis->device = (1 << 6);
	fis->lba0 = (uint8_t)(lba);
	fis->lba1 = (uint8_t)(lba >> 8);
	fis->lba2 = (uint8_t)(lba >> 16);
	fis->lba3 = (uint8_t)(lba >> 24);
	fis->lba4 = (uint8_t)(lba >> 32);
	fis->lba5 = (uint8_t)(lba >> 40);
	fis->count = count;
}

static void setup_prdt(struct ahci_cmd_tbl *tbl, struct ahci_cmd_hdr *hdr,
		       uint64_t buf_phys, uint32_t byte_count, uint16_t opts)
{
	tbl->prdt[0].dba = (uint32_t)(buf_phys);
	tbl->prdt[0].dbau = (uint32_t)(buf_phys >> 32);
	tbl->prdt[0].dbc = (byte_count - 1) | 1;
	hdr->opts = opts;
	hdr->prdtl = 1;
	hdr->prdbc = 0;
}

int ahci_cmd_identify(struct ahci_port *port, void *buf)
{
	if (!port || !buf)
		return -EINVAL;

	uint64_t hhdm = vmm_get_hhdm();

	build_h2d_fis(port->cmd_tbl, ATA_CMD_IDENTIFY, 0, 0);
	setup_prdt(port->cmd_tbl, &port->cmd_list[0],
		   ahci_virt_to_phys(buf, hhdm), AHCI_SECTOR_SIZE,
		   (uint16_t)AHCI_CFL_H2D);

	return ahci_submit_slot0(port);
}

int ahci_cmd_read_dma(struct ahci_port *port, uint64_t lba,
		      uint32_t count, void *buf)
{
	if (!port || !buf || count == 0)
		return -EINVAL;

	uint64_t hhdm = vmm_get_hhdm();

	build_h2d_fis(port->cmd_tbl, ATA_CMD_READ_DMA_EXT,
		      lba, (uint16_t)count);
	setup_prdt(port->cmd_tbl, &port->cmd_list[0],
		   ahci_virt_to_phys(buf, hhdm), count * AHCI_SECTOR_SIZE,
		   (uint16_t)AHCI_CFL_H2D);

	return ahci_submit_slot0(port);
}

int ahci_cmd_write_dma(struct ahci_port *port, uint64_t lba,
		       uint32_t count, const void *buf)
{
	if (!port || !buf || count == 0)
		return -EINVAL;

	uint64_t hhdm = vmm_get_hhdm();

	build_h2d_fis(port->cmd_tbl, ATA_CMD_WRITE_DMA_EXT,
		      lba, (uint16_t)count);
	setup_prdt(port->cmd_tbl, &port->cmd_list[0],
		   ahci_virt_to_phys((void *)(uintptr_t)buf, hhdm),
		   count * AHCI_SECTOR_SIZE,
		   (uint16_t)(AHCI_CFL_H2D | AHCI_CMD_WRITE_BIT));

	return ahci_submit_slot0(port);
}

#endif /* CONFIG_AHCI */
