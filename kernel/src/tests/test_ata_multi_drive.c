#include "ktest.h"
#include "../drivers/new/ata/ata.h"

KTEST_REGISTER(test_ata_multi_drive, "ata multi drive query", KTEST_CAT_BOOT)
static void test_ata_multi_drive(void)
{
	KTEST_BEGIN("ATA Multi-Drive Boundary Check");

	/* Get drive 4 - should be NULL since ATA_MAX_DRIVES = 4 (indices 0-3) */
	struct ata_drive *out_of_bounds = ata_get_drive(4);
	KTEST_NULL(out_of_bounds, "Getting drive >= ATA_MAX_DRIVES fails gracefully");
	
	int count = ata_drive_count();
	KTEST_TRUE(count >= 0 && count <= 4, "Drive count must be well-formed");

#ifdef CONFIG_TEST_ATA_DRIVE
	KTEST_TRUE(count >= 1, "At least one drive reported when attached");
#else
	KTEST_TRUE(1, "Bypassed multi drive check (no disk assumed)");
#endif
}
