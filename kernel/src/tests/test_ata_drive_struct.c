#include "ktest.h"
#include "drivers/new/ata/ata.h"
#include "lib/string.h"

KTEST_REGISTER(test_ata_drive_struct, "ATA: drive struct", KTEST_CAT_BOOT)
static void test_ata_drive_struct(void)
{
	KTEST_BEGIN("ATA: drive struct");

	struct ata_drive drv;
	memset(&drv, 0, sizeof(drv));

	KTEST_EQ(drv.present, 0, "zeroed present");
	KTEST_EQ(drv.bus, 0, "zeroed bus");
	KTEST_EQ(drv.drive, 0, "zeroed drive");
	KTEST_EQ(drv.io_base, 0, "zeroed io_base");
	KTEST_EQ(drv.lba28_sectors, 0, "zeroed lba28");
	KTEST_EQ(drv.lba48, 0, "zeroed lba48 flag");
	KTEST_EQ(drv.lba48_sectors, 0, "zeroed lba48 sectors");
	KTEST_EQ(drv.model[0], 0, "zeroed model");

	drv.present = 1;
	drv.bus = 0;
	drv.drive = 1;
	drv.io_base = ATA_PRI_DATA;
	drv.ctrl_base = ATA_PRI_ALT_STATUS;
	drv.drive_sel = ATA_DRIVE_SLAVE;
	drv.lba28_sectors = 32768;
	memcpy(drv.model, "TestDisk", 9);

	KTEST_EQ(drv.present, 1, "set present");
	KTEST_EQ(drv.drive_sel, ATA_DRIVE_SLAVE, "set slave");
	KTEST_EQ(drv.lba28_sectors, 32768, "set sector count");
	KTEST_TRUE(memcmp(drv.model, "TestDisk", 8) == 0, "model string");
	KTEST_EQ(drv.model[8], '\0', "model null term");
}
