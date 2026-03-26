#include "ktest.h"
#include "drivers/new/ata/ata.h"
#include "lib/string.h"

KTEST_REGISTER(test_ata_model_string, "ATA: model string format", KTEST_CAT_BOOT)
static void test_ata_model_string(void)
{
	KTEST_BEGIN("ATA: model string format");

	if (ata_drive_count() == 0)
		return;

	struct ata_drive *drv = ata_get_drive(0);
	if (!drv) return;

	size_t len = strlen(drv->model);
	KTEST_GT(len, 0, "model not empty");
	KTEST_LT(len, 41, "model within bounds");

	/* No trailing spaces (should be trimmed) */
	if (len > 0)
		KTEST_NE(drv->model[len - 1], ' ', "no trailing space");

	/* Null terminated */
	KTEST_EQ(drv->model[len], '\0', "null terminated");
}
