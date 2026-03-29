#include "ext2.h"
#include <dicron/mem.h>
#include <dicron/log.h>
#include "lib/string.h"
#include "drivers/timer/rtc.h"

/* ── Dirty tracking bitmaps ── */

#define DIRTY_BITMAP_SIZE 64  /* supports up to 512 inodes/groups tracked */

static uint8_t dirty_inodes[DIRTY_BITMAP_SIZE];
static uint8_t dirty_bgs[DIRTY_BITMAP_SIZE];
static int sb_dirty;

static void set_bit(uint8_t *bitmap, uint32_t idx)
{
	if (idx / 8 < DIRTY_BITMAP_SIZE)
		bitmap[idx / 8] |= (uint8_t)(1U << (idx % 8));
}

static int test_bit(const uint8_t *bitmap, uint32_t idx)
{
	if (idx / 8 >= DIRTY_BITMAP_SIZE)
		return 0;
	return (bitmap[idx / 8] >> (idx % 8)) & 1;
}

static void clear_bitmap(uint8_t *bitmap)
{
	memset(bitmap, 0, DIRTY_BITMAP_SIZE);
}

void ext2_mark_inode_dirty(struct ext2_fs *fs, uint32_t ino)
{
	(void)fs;
	if (ino > 0)
		set_bit(dirty_inodes, ino);
}

void ext2_mark_bg_dirty(struct ext2_fs *fs, uint32_t group)
{
	(void)fs;
	set_bit(dirty_bgs, group);
}

void ext2_mark_sb_dirty(struct ext2_fs *fs)
{
	(void)fs;
	sb_dirty = 1;
}

/* ── Sync all dirty metadata to disk ── */

int ext2_sync(struct ext2_fs *fs)
{
	if (!fs)
		return -1;

	int errors = 0;

	/* Flush dirty inodes */
	uint32_t max_tracked = DIRTY_BITMAP_SIZE * 8;
	uint32_t limit = fs->sb.s_inodes_count;
	if (limit > max_tracked)
		limit = max_tracked;

	for (uint32_t ino = 1; ino <= limit; ino++) {
		if (!test_bit(dirty_inodes, ino))
			continue;

		struct ext2_inode inode;
		if (ext2_read_inode(fs, ino, &inode) != 0) {
			klog(KLOG_ERR, "ext2_sync: failed to read "
			     "dirty inode %u\n", ino);
			errors++;
			continue;
		}

		if (ext2_write_inode(fs, ino, &inode) != 0) {
			klog(KLOG_ERR, "ext2_sync: failed to write "
			     "dirty inode %u\n", ino);
			errors++;
		}
	}

	/* Flush dirty block group descriptors */
	for (uint32_t g = 0; g < fs->groups_count; g++) {
		if (test_bit(dirty_bgs, g)) {
			if (ext2_write_group_descs(fs) != 0) {
				klog(KLOG_ERR, "ext2_sync: failed to write "
				     "group descriptors\n");
				errors++;
			}
			break;
		}
	}

	/* Flush superblock */
	if (sb_dirty) {
		fs->sb.s_wtime = (uint32_t)rtc_unix_time();
		fs->sb.s_mnt_count++;
		if (ext2_write_superblock(fs) != 0) {
			klog(KLOG_ERR, "ext2_sync: failed to write "
			     "superblock\n");
			errors++;
		}
	}

	/* Clear all dirty bits */
	clear_bitmap(dirty_inodes);
	clear_bitmap(dirty_bgs);
	sb_dirty = 0;

	if (errors == 0)
		klog(KLOG_DEBUG, "ext2: sync complete\n");
	else
		klog(KLOG_ERR, "ext2: sync completed with %d error(s)\n",
		     errors);

	return errors;
}

/* ── Clean unmount ── */

void ext2_sync_unmount(struct ext2_fs *fs)
{
	if (!fs)
		return;

	/* Flush all pending writes */
	ext2_sync(fs);

	/* Mark filesystem as cleanly unmounted */
	fs->sb.s_state = EXT2_VALID_FS;
	fs->sb.s_wtime = (uint32_t)rtc_unix_time();
	ext2_write_superblock(fs);

	klog(KLOG_DEBUG, "ext2: clean unmount complete\n");
}
