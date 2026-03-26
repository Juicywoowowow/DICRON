#include "ktest.h"
#include "drivers/new/ata/ata.h"

KTEST_REGISTER(test_ata_port_offsets, "ATA: port register offsets", KTEST_CAT_BOOT)
static void test_ata_port_offsets(void)
{
	KTEST_BEGIN("ATA: port register offsets");

	/* Primary bus: base 0x1F0, registers at +0 through +7 */
	KTEST_EQ(ATA_PRI_ERROR,      ATA_PRI_DATA + 1, "pri error +1");
	KTEST_EQ(ATA_PRI_SECT_COUNT, ATA_PRI_DATA + 2, "pri sect_count +2");
	KTEST_EQ(ATA_PRI_LBA_LO,    ATA_PRI_DATA + 3, "pri lba_lo +3");
	KTEST_EQ(ATA_PRI_LBA_MID,   ATA_PRI_DATA + 4, "pri lba_mid +4");
	KTEST_EQ(ATA_PRI_LBA_HI,    ATA_PRI_DATA + 5, "pri lba_hi +5");
	KTEST_EQ(ATA_PRI_DRIVE_HEAD, ATA_PRI_DATA + 6, "pri drive_head +6");
	KTEST_EQ(ATA_PRI_STATUS,    ATA_PRI_DATA + 7, "pri status +7");
	KTEST_EQ(ATA_PRI_COMMAND,   ATA_PRI_STATUS, "pri cmd == status");

	/* Secondary bus: base 0x170 */
	KTEST_EQ(ATA_SEC_ERROR,      ATA_SEC_DATA + 1, "sec error +1");
	KTEST_EQ(ATA_SEC_SECT_COUNT, ATA_SEC_DATA + 2, "sec sect_count +2");
	KTEST_EQ(ATA_SEC_LBA_LO,    ATA_SEC_DATA + 3, "sec lba_lo +3");
	KTEST_EQ(ATA_SEC_LBA_MID,   ATA_SEC_DATA + 4, "sec lba_mid +4");
	KTEST_EQ(ATA_SEC_LBA_HI,    ATA_SEC_DATA + 5, "sec lba_hi +5");
	KTEST_EQ(ATA_SEC_DRIVE_HEAD, ATA_SEC_DATA + 6, "sec drive_head +6");
	KTEST_EQ(ATA_SEC_STATUS,    ATA_SEC_DATA + 7, "sec status +7");
}
