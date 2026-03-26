#include "ktest.h"
#include "drivers/ata/ata.h"
#include "lib/string.h"

KTEST_REGISTER(test_ata_pio_null, "ATA: PIO null/bad args", KTEST_CAT_BOOT)
static void test_ata_pio_null(void)
{
	KTEST_BEGIN("ATA: PIO null/bad args");

	uint8_t buf[512];
	struct ata_drive fake;
	memset(&fake, 0, sizeof(fake));

	KTEST_EQ(ata_pio_read(NULL, 0, 1, buf), -1, "read NULL drv");
	KTEST_EQ(ata_pio_write(NULL, 0, 1, buf), -1, "write NULL drv");

	KTEST_EQ(ata_pio_read(&fake, 0, 1, buf), -1,
		 "read non-present drv");
	KTEST_EQ(ata_pio_write(&fake, 0, 1, buf), -1,
		 "write non-present drv");

	fake.present = 1;
	KTEST_EQ(ata_pio_read(&fake, 0, 1, NULL), -1, "read NULL buf");
	KTEST_EQ(ata_pio_write(&fake, 0, 1, NULL), -1, "write NULL buf");
	KTEST_EQ(ata_pio_read(&fake, 0, 0, buf), -1, "read zero count");
	KTEST_EQ(ata_pio_write(&fake, 0, 0, buf), -1, "write zero count");
}
