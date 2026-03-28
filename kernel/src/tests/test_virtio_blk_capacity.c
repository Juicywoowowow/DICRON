/*
 * test_virtio_blk_capacity.c — virtio-blk capacity and block-size tests.
 *
 * When a virtio-blk device is present these tests verify that the
 * reported capacity and block size match QEMU's minimum requirements.
 * Tests skip gracefully when no virtio disk is attached.
 */

#include "ktest.h"
#include "../drivers/new/virtio/virtio_blk.h"
#include <dicron/blkdev.h>

KTEST_REGISTER(ktest_virtio_blk_cap_nonzero, "virtio-blk: total_blocks > 0 when present", KTEST_CAT_BOOT)
static void ktest_virtio_blk_cap_nonzero(void)
{
	KTEST_BEGIN("virtio-blk: total_blocks > 0 when present");
	if (virtio_blk_count() == 0) {
		KTEST_TRUE(1, "virtio-blk: no device (skip)");
		return;
	}
	struct blkdev *bd = virtio_blk_get_device(0);
	KTEST_NOT_NULL(bd, "virtio-blk: blkdev[0] non-null");
	KTEST(bd->total_blocks > (uint64_t)0, "virtio-blk: total_blocks > 0");
}

KTEST_REGISTER(ktest_virtio_blk_cap_min_1mb, "virtio-blk: capacity >= 1 MB (2048 sectors)", KTEST_CAT_BOOT)
static void ktest_virtio_blk_cap_min_1mb(void)
{
	KTEST_BEGIN("virtio-blk: capacity >= 1 MB (2048 sectors)");
	if (virtio_blk_count() == 0) {
		KTEST_TRUE(1, "virtio-blk: no device (skip)");
		return;
	}
	struct blkdev *bd = virtio_blk_get_device(0);
	KTEST_NOT_NULL(bd, "virtio-blk: blkdev[0] non-null");
	/* QEMU minimum virtio-blk image is 1 MB = 2048 × 512-byte sectors */
	KTEST(bd->total_blocks >= (uint64_t)2048,
	      "virtio-blk: capacity >= 2048 sectors");
}

KTEST_REGISTER(ktest_virtio_blk_block_size_512, "virtio-blk: block_size == 512", KTEST_CAT_BOOT)
static void ktest_virtio_blk_block_size_512(void)
{
	KTEST_BEGIN("virtio-blk: block_size == 512");
	if (virtio_blk_count() == 0) {
		KTEST_TRUE(1, "virtio-blk: no device (skip)");
		return;
	}
	struct blkdev *bd = virtio_blk_get_device(0);
	KTEST_NOT_NULL(bd, "virtio-blk: blkdev[0] non-null");
	KTEST_EQ((int)bd->block_size, 512, "virtio-blk: block_size is 512");
}

KTEST_REGISTER(ktest_virtio_blk_cap_consistent, "virtio-blk: capacity consistent across calls", KTEST_CAT_BOOT)
static void ktest_virtio_blk_cap_consistent(void)
{
	KTEST_BEGIN("virtio-blk: capacity consistent across calls");
	if (virtio_blk_count() == 0) {
		KTEST_TRUE(1, "virtio-blk: no device (skip)");
		return;
	}
	struct blkdev *bd1 = virtio_blk_get_device(0);
	struct blkdev *bd2 = virtio_blk_get_device(0);
	KTEST_NOT_NULL(bd1, "virtio-blk: first get_device(0) non-null");
	KTEST_NOT_NULL(bd2, "virtio-blk: second get_device(0) non-null");
	KTEST(bd1->total_blocks == bd2->total_blocks,
	      "virtio-blk: capacity same on repeated get_device(0)");
}
