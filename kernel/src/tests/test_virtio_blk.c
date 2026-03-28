#include "ktest.h"
#include "../drivers/new/virtio/virtio_blk.h"
#include <dicron/blkdev.h>

KTEST_REGISTER(ktest_virtio_blk_count, "virtio-blk: device count is non-negative", KTEST_CAT_BOOT)
static void ktest_virtio_blk_count(void)
{
	KTEST_BEGIN("virtio-blk: device count is non-negative");
	int count = virtio_blk_count();
	KTEST_GE(count, 0, "virtio-blk: count >= 0");
}

KTEST_REGISTER(ktest_virtio_blk_blkdev, "virtio-blk: blkdev fields valid when present", KTEST_CAT_BOOT)
static void ktest_virtio_blk_blkdev(void)
{
	KTEST_BEGIN("virtio-blk: blkdev fields valid when present");

	if (virtio_blk_count() == 0) {
		/* No virtio disk in this QEMU run — skip gracefully */
		KTEST_TRUE(1, "virtio-blk: no device (skip)");
		return;
	}

	struct blkdev *bd = virtio_blk_get_device(0);
	KTEST_NOT_NULL(bd, "virtio-blk: blkdev[0] is non-null");
	KTEST_NOT_NULL(bd->read,  "virtio-blk: blkdev read op present");
	KTEST_NOT_NULL(bd->write, "virtio-blk: blkdev write op present");
	KTEST(bd->block_size == 512, "virtio-blk: block size is 512");
	KTEST(bd->total_blocks > (uint64_t)0, "virtio-blk: capacity > 0");
}
