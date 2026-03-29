#ifndef _DICRON_DRIVERS_AHCI_H
#define _DICRON_DRIVERS_AHCI_H

#include <stdint.h>

#define AHCI_MAX_PORTS		32
#define AHCI_SECTOR_SIZE	512

#define AHCI_SIG_SATA		0x00000101
#define AHCI_SIG_ATAPI		0xEB140101

#define AHCI_SSTS_DET_MASK	0x0F
#define AHCI_SSTS_DET_PRESENT	0x03

#define AHCI_CMD_ST		(1U << 0)
#define AHCI_CMD_FRE		(1U << 4)
#define AHCI_CMD_FR		(1U << 14)
#define AHCI_CMD_CR		(1U << 15)

#define AHCI_GHC_AE		(1U << 31)

#define AHCI_BOHC_BOS		(1U << 0)
#define AHCI_BOHC_OOS		(1U << 1)

#define AHCI_TFD_BSY		(1U << 7)
#define AHCI_TFD_DRQ		(1U << 3)
#define AHCI_TFD_ERR		(1U << 0)

#define AHCI_PxIS_TFES		(1U << 30)

#define FIS_TYPE_REG_H2D	0x27

#define ATA_CMD_IDENTIFY	0xEC
#define ATA_CMD_READ_DMA_EXT	0x25
#define ATA_CMD_WRITE_DMA_EXT	0x35

/* ── HBA registers (MMIO) ── */

struct ahci_port_regs {
	volatile uint32_t clb;
	volatile uint32_t clbu;
	volatile uint32_t fb;
	volatile uint32_t fbu;
	volatile uint32_t is;
	volatile uint32_t ie;
	volatile uint32_t cmd;
	volatile uint32_t rsv0;
	volatile uint32_t tfd;
	volatile uint32_t sig;
	volatile uint32_t ssts;
	volatile uint32_t sctl;
	volatile uint32_t serr;
	volatile uint32_t sact;
	volatile uint32_t ci;
	volatile uint32_t sntf;
	volatile uint32_t fbs;
	volatile uint32_t rsv1[11];
	volatile uint32_t vs[4];
} __attribute__((packed));

struct ahci_hba_regs {
	volatile uint32_t cap;
	volatile uint32_t ghc;
	volatile uint32_t is;
	volatile uint32_t pi;
	volatile uint32_t vs;
	volatile uint32_t ccc_ctl;
	volatile uint32_t ccc_ports;
	volatile uint32_t em_loc;
	volatile uint32_t em_ctl;
	volatile uint32_t cap2;
	volatile uint32_t bohc;
	volatile uint8_t  rsv[0xA0 - 0x2C];
	volatile uint8_t  vendor[0x100 - 0xA0];
	struct ahci_port_regs ports[AHCI_MAX_PORTS];
} __attribute__((packed));

struct ahci_cmd_hdr {
	uint16_t opts;
	uint16_t prdtl;
	volatile uint32_t prdbc;
	uint32_t ctba;
	uint32_t ctbau;
	uint32_t rsv[4];
} __attribute__((packed));

struct ahci_prdt_entry {
	uint32_t dba;
	uint32_t dbau;
	uint32_t rsv;
	uint32_t dbc;
} __attribute__((packed));

struct fis_reg_h2d {
	uint8_t  type;
	uint8_t  pmport_c;
	uint8_t  command;
	uint8_t  feature_lo;
	uint8_t  lba0;
	uint8_t  lba1;
	uint8_t  lba2;
	uint8_t  device;
	uint8_t  lba3;
	uint8_t  lba4;
	uint8_t  lba5;
	uint8_t  feature_hi;
	uint16_t count;
	uint8_t  icc;
	uint8_t  control;
	uint8_t  rsv[4];
} __attribute__((packed));

struct ahci_cmd_tbl {
	uint8_t cfis[64];
	uint8_t acmd[16];
	uint8_t rsv[48];
	struct ahci_prdt_entry prdt[1];
} __attribute__((packed));

struct ahci_port {
	int                    port_num;
	struct ahci_port_regs *regs;
	struct ahci_cmd_hdr   *cmd_list;
	void                  *fis_base;
	struct ahci_cmd_tbl   *cmd_tbl;
};

/* Shared helper */
static inline uint64_t ahci_virt_to_phys(void *virt, uint64_t hhdm)
{
	return (uint64_t)(uintptr_t)virt - hhdm;
}

void              ahci_init(void);
int               ahci_port_count(void);
struct ahci_port *ahci_get_port(int index);

int ahci_cmd_identify(struct ahci_port *port, void *buf);
int ahci_cmd_read_dma(struct ahci_port *port, uint64_t lba,
		      uint32_t count, void *buf);
int ahci_cmd_write_dma(struct ahci_port *port, uint64_t lba,
		       uint32_t count, const void *buf);

#endif
