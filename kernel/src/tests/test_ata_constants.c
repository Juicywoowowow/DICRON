#include "ktest.h"
#include "drivers/new/ata/ata.h"

KTEST_REGISTER(test_ata_constants, "ATA: port/cmd constants", KTEST_CAT_BOOT)
static void test_ata_constants(void)
{
	KTEST_BEGIN("ATA: port/cmd constants");

	KTEST_EQ(ATA_PRI_DATA, 0x1F0, "primary data port");
	KTEST_EQ(ATA_PRI_STATUS, 0x1F7, "primary status port");
	KTEST_EQ(ATA_PRI_ALT_STATUS, 0x3F6, "primary alt status");
	KTEST_EQ(ATA_SEC_DATA, 0x170, "secondary data port");
	KTEST_EQ(ATA_SEC_STATUS, 0x177, "secondary status port");
	KTEST_EQ(ATA_SEC_ALT_STATUS, 0x376, "secondary alt status");

	KTEST_EQ(ATA_SR_BSY, 0x80, "BSY bit");
	KTEST_EQ(ATA_SR_DRDY, 0x40, "DRDY bit");
	KTEST_EQ(ATA_SR_DRQ, 0x08, "DRQ bit");
	KTEST_EQ(ATA_SR_ERR, 0x01, "ERR bit");
	KTEST_EQ(ATA_SR_DF, 0x20, "DF bit");

	KTEST_EQ(ATA_CMD_IDENTIFY, 0xEC, "IDENTIFY cmd");
	KTEST_EQ(ATA_CMD_READ_SECTORS, 0x20, "READ_SECTORS cmd");
	KTEST_EQ(ATA_CMD_WRITE_SECTORS, 0x30, "WRITE_SECTORS cmd");
	KTEST_EQ(ATA_CMD_CACHE_FLUSH, 0xE7, "CACHE_FLUSH cmd");

	KTEST_EQ(ATA_SECTOR_SIZE, 512, "sector size");
	KTEST_EQ(ATA_DRIVE_MASTER, 0xE0, "master select");
	KTEST_EQ(ATA_DRIVE_SLAVE, 0xF0, "slave select");
	KTEST_EQ(ATA_MAX_DRIVES, 4, "max drives");
}
