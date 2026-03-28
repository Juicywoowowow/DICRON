/*
 * test_virtio_blk_private.c — virtio-blk internal state and ops tests.
 *
 * Confirms that blkdev->private is set, that read/write function pointers
 * are non-null, and that both ops reject NULL buffer arguments correctly.
 */

#include "ktest.h"
#include "../drivers/new/virtio/virtio_blk.h"
#include <dicron/blkdev.h>
#include <dicron/errno.h>

KTEST_REGISTER(ktest_virtio_blk_private_nonnull, "virtio-blk: blkdev->private is non-null", KTEST_CAT_BOOT)
static void ktest_virtio_blk_private_nonnull(void)
{
	KTEST_BEGIN("virtio-blk: blkdev->private is non-null");
	if (virtio_blk_count() == 0) {
		KTEST_TRUE(1, "virtio-blk: no device (skip)");
		return;
	}
	struct blkdev *bd = virtio_blk_get_device(0);
	KTEST_NOT_NULL(bd, "virtio-blk: blkdev[0] non-null");
	KTEST_NOT_NULL(bd->private, "virtio-blk: blkdev->private non-null");
}

KTEST_REGISTER(ktest_virtio_blk_ops_nonnull, "virtio-blk: read and write ops are non-null", KTEST_CAT_BOOT)
static void ktest_virtio_blk_ops_nonnull(void)
{
	KTEST_BEGIN("virtio-blk: read and write ops are non-null");
	if (virtio_blk_count() == 0) {
		KTEST_TRUE(1, "virtio-blk: no device (skip)");
		return;
	}
	struct blkdev *bd = virtio_blk_get_device(0);
	KTEST_NOT_NULL(bd, "virtio-blk: blkdev[0] non-null");
	KTEST_NOT_NULL(bd->read,  "virtio-blk: read op non-null");
	KTEST_NOT_NULL(bd->write, "virtio-blk: write op non-null");
}

KTEST_REGISTER(ktest_virtio_blk_read_null_buf, "virtio-blk: read(NULL buf) returns -EINVAL", KTEST_CAT_BOOT)
static void ktest_virtio_blk_read_null_buf(void)
{
	KTEST_BEGIN("virtio-blk: read(NULL buf) returns -EINVAL");
	if (virtio_blk_count() == 0) {
		KTEST_TRUE(1, "virtio-blk: no device (skip)");
		return;
	}
	struct blkdev *bd = virtio_blk_get_device(0);
	KTEST_NOT_NULL(bd, "virtio-blk: blkdev[0] non-null");
	int ret = bd->read(bd, 0, NULL);
	KTEST_LT(ret, 0, "virtio-blk: read(NULL) returns negative errno");
}

KTEST_REGISTER(ktest_virtio_blk_write_null_buf, "virtio-blk: write(NULL buf) returns -EINVAL", KTEST_CAT_BOOT)
static void ktest_virtio_blk_write_null_buf(void)
{
	KTEST_BEGIN("virtio-blk: write(NULL buf) returns -EINVAL");
	if (virtio_blk_count() == 0) {
		KTEST_TRUE(1, "virtio-blk: no device (skip)");
		return;
	}
	struct blkdev *bd = virtio_blk_get_device(0);
	KTEST_NOT_NULL(bd, "virtio-blk: blkdev[0] non-null");
	int ret = bd->write(bd, 0, NULL);
	KTEST_LT(ret, 0, "virtio-blk: write(NULL) returns negative errno");
}
