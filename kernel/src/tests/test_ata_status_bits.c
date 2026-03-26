#include "ktest.h"
#include "drivers/new/ata/ata.h"

KTEST_REGISTER(test_ata_status_bits, "ATA: status bit isolation", KTEST_CAT_BOOT)
static void test_ata_status_bits(void)
{
	KTEST_BEGIN("ATA: status bit isolation");

	/* Each bit should be a unique power of two */
	KTEST_EQ(ATA_SR_ERR  & ATA_SR_DRQ, 0, "ERR vs DRQ");
	KTEST_EQ(ATA_SR_ERR  & ATA_SR_BSY, 0, "ERR vs BSY");
	KTEST_EQ(ATA_SR_DRQ  & ATA_SR_BSY, 0, "DRQ vs BSY");
	KTEST_EQ(ATA_SR_DRDY & ATA_SR_BSY, 0, "DRDY vs BSY");
	KTEST_EQ(ATA_SR_DF   & ATA_SR_DRQ, 0, "DF vs DRQ");

	/* Combined masks */
	uint8_t all = ATA_SR_BSY | ATA_SR_DRDY | ATA_SR_DF |
		      ATA_SR_DSC | ATA_SR_DRQ | ATA_SR_CORR |
		      ATA_SR_IDX | ATA_SR_ERR;
	KTEST_EQ(all, 0xFF, "all bits cover full byte");

	/* Test masking a fake status */
	uint8_t status = ATA_SR_DRDY | ATA_SR_DRQ;
	KTEST_TRUE(status & ATA_SR_DRDY, "DRDY set in combo");
	KTEST_TRUE(status & ATA_SR_DRQ, "DRQ set in combo");
	KTEST_FALSE(status & ATA_SR_BSY, "BSY not set");
	KTEST_FALSE(status & ATA_SR_ERR, "ERR not set");
}
