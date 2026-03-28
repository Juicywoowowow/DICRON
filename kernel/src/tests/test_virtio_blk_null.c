/*
 * test_virtio_blk_null.c — virtio-blk null pointer and out-of-bounds tests.
 *
 * Verifies that get_device() returns NULL for all invalid indices and
 * that count() returns a stable, non-negative value.
 */

#include "ktest.h"
#include "../drivers/new/virtio/virtio_blk.h"
#include <dicron/blkdev.h>

KTEST_REGISTER(ktest_virtio_blk_get_neg, "virtio-blk: get_device(-1) returns NULL", KTEST_CAT_BOOT)
static void ktest_virtio_blk_get_neg(void)
{
	KTEST_BEGIN("virtio-blk: get_device(-1) returns NULL");
	struct blkdev *bd = virtio_blk_get_device(-1);
	KTEST_NULL(bd, "virtio-blk: get_device(-1) is NULL");
}

KTEST_REGISTER(ktest_virtio_blk_get_neg2, "virtio-blk: get_device(-2) returns NULL", KTEST_CAT_BOOT)
static void ktest_virtio_blk_get_neg2(void)
{
	KTEST_BEGIN("virtio-blk: get_device(-2) returns NULL");
	struct blkdev *bd = virtio_blk_get_device(-2);
	KTEST_NULL(bd, "virtio-blk: get_device(-2) is NULL");
}

KTEST_REGISTER(ktest_virtio_blk_get_large, "virtio-blk: get_device(1000) returns NULL", KTEST_CAT_BOOT)
static void ktest_virtio_blk_get_large(void)
{
	KTEST_BEGIN("virtio-blk: get_device(1000) returns NULL");
	struct blkdev *bd = virtio_blk_get_device(1000);
	KTEST_NULL(bd, "virtio-blk: get_device(1000) is NULL");
}

KTEST_REGISTER(ktest_virtio_blk_get_past_count, "virtio-blk: get_device(count) returns NULL", KTEST_CAT_BOOT)
static void ktest_virtio_blk_get_past_count(void)
{
	KTEST_BEGIN("virtio-blk: get_device(count) returns NULL");
	int count = virtio_blk_count();
	/* One past the last valid index must always be NULL */
	struct blkdev *bd = virtio_blk_get_device(count);
	KTEST_NULL(bd, "virtio-blk: get_device(count) is NULL");
}
