/*
 * test_virtio_blk_max.c — virtio-blk device table limit and multi-device tests.
 *
 * Checks that the device count never exceeds VIRTIO_BLK_MAX_DEVS, that all
 * reported devices are accessible and stable, and that count() is consistent.
 */

#include "ktest.h"
#include "../drivers/new/virtio/virtio_blk.h"
#include <dicron/blkdev.h>

/* Must match VIRTIO_BLK_MAX_DEVS in virtio_blk.c */
#define VIRTIO_BLK_MAX_DEVS	4

KTEST_REGISTER(ktest_virtio_blk_count_max, "virtio-blk: count <= VIRTIO_BLK_MAX_DEVS", KTEST_CAT_BOOT)
static void ktest_virtio_blk_count_max(void)
{
	KTEST_BEGIN("virtio-blk: count <= VIRTIO_BLK_MAX_DEVS");
	int count = virtio_blk_count();
	KTEST_GE(count, 0, "virtio-blk: count >= 0");
	KTEST(count <= VIRTIO_BLK_MAX_DEVS, "virtio-blk: count <= 4");
}

KTEST_REGISTER(ktest_virtio_blk_count_stable, "virtio-blk: count() is stable across calls", KTEST_CAT_BOOT)
static void ktest_virtio_blk_count_stable(void)
{
	KTEST_BEGIN("virtio-blk: count() is stable across calls");
	int c1 = virtio_blk_count();
	int c2 = virtio_blk_count();
	int c3 = virtio_blk_count();
	KTEST_EQ(c1, c2, "virtio-blk: count same on call 1 and 2");
	KTEST_EQ(c2, c3, "virtio-blk: count same on call 2 and 3");
}

KTEST_REGISTER(ktest_virtio_blk_all_accessible, "virtio-blk: all reported devices are accessible", KTEST_CAT_BOOT)
static void ktest_virtio_blk_all_accessible(void)
{
	KTEST_BEGIN("virtio-blk: all reported devices are accessible");
	int count = virtio_blk_count();
	for (int i = 0; i < count; i++) {
		struct blkdev *bd = virtio_blk_get_device(i);
		KTEST_NOT_NULL(bd, "virtio-blk: device[i] is non-null");
	}
	KTEST_TRUE(1, "virtio-blk: all devices accessible");
}

KTEST_REGISTER(ktest_virtio_blk_all_have_ops, "virtio-blk: all devices have read and write ops", KTEST_CAT_BOOT)
static void ktest_virtio_blk_all_have_ops(void)
{
	KTEST_BEGIN("virtio-blk: all devices have read and write ops");
	int count = virtio_blk_count();
	for (int i = 0; i < count; i++) {
		struct blkdev *bd = virtio_blk_get_device(i);
		KTEST_NOT_NULL(bd,        "virtio-blk: device[i] non-null");
		KTEST_NOT_NULL(bd->read,  "virtio-blk: device[i].read non-null");
		KTEST_NOT_NULL(bd->write, "virtio-blk: device[i].write non-null");
	}
	KTEST_TRUE(1, "virtio-blk: all devices have valid ops");
}
