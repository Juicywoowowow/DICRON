#include "ext2_test_helper.h"
#include "lib/string.h"
#include <dicron/mem.h>

/*
 * 64 KiB ramdisk image, 1024-byte blocks, 64 blocks total.
 *
 * Layout:
 *   Block 0  : boot (zeroed)
 *   Block 1  : superblock
 *   Block 2  : group descriptor table (1 group)
 *   Block 3  : block bitmap
 *   Block 4  : inode bitmap
 *   Block 5-8: inode table (4 blocks × 8 inodes/block = 32 inodes)
 *   Block 9  : root directory data
 *   Block 10+: free data blocks
 */

#define TEST_IMAGE_SIZE (64 * 1024)
#define TEST_BLOCK_SIZE 1024
#define TEST_TOTAL_BLOCKS 64
#define TEST_INODES_COUNT 32 /* 4 inode-table blocks × (1024/128) */

/* Block assignments */
#define BLK_BOOT 0
#define BLK_SUPER 1
#define BLK_BGD 2
#define BLK_BBITMAP 3
#define BLK_IBITMAP 4
#define BLK_ITABLE 5 /* 5,6,7,8 = 4 blocks */
#define BLK_ROOTDIR 9
#define BLK_FIRST_FREE 10

/* Persistent backing buffer — static so it survives the function call */
static uint8_t test_image[TEST_IMAGE_SIZE];

static void write_block(uint8_t *img, uint32_t block, const void *data,
                        uint32_t size) {
  memcpy(img + block * TEST_BLOCK_SIZE, data, size);
}

struct ext2_fs *ext2_test_setup(void) {
  memset(test_image, 0, TEST_IMAGE_SIZE);

  /* ── Superblock (block 1) ── */
  struct ext2_superblock sb;
  memset(&sb, 0, sizeof(sb));
  sb.s_inodes_count = TEST_INODES_COUNT;
  sb.s_blocks_count = TEST_TOTAL_BLOCKS;
  sb.s_r_blocks_count = 0;
  sb.s_free_blocks_count = TEST_TOTAL_BLOCKS - BLK_FIRST_FREE;
  sb.s_free_inodes_count = TEST_INODES_COUNT - 2; /* inodes 1,2 used */
  sb.s_first_data_block = 1;                      /* for 1024-byte blocks */
  sb.s_log_block_size = 0;                        /* 1024 << 0 = 1024 */
  sb.s_log_frag_size = 0;
  sb.s_blocks_per_group = TEST_TOTAL_BLOCKS;
  sb.s_frags_per_group = TEST_TOTAL_BLOCKS;
  sb.s_inodes_per_group = TEST_INODES_COUNT;
  sb.s_magic = EXT2_MAGIC;
  sb.s_state = 1; /* EXT2_VALID_FS */
  sb.s_rev_level = 0;
  sb.s_first_ino = 11;
  sb.s_inode_size = 128;

  write_block(test_image, BLK_SUPER, &sb, sizeof(sb));

  /* ── Block Group Descriptor (block 2) ── */
  struct ext2_group_desc gd;
  memset(&gd, 0, sizeof(gd));
  gd.bg_block_bitmap = BLK_BBITMAP;
  gd.bg_inode_bitmap = BLK_IBITMAP;
  gd.bg_inode_table = BLK_ITABLE;
  gd.bg_free_blocks_count = (uint16_t)(TEST_TOTAL_BLOCKS - BLK_FIRST_FREE);
  gd.bg_free_inodes_count = (uint16_t)(TEST_INODES_COUNT - 2);
  gd.bg_used_dirs_count = 1; /* root dir */

  write_block(test_image, BLK_BGD, &gd, sizeof(gd));
  uint8_t bbitmap[TEST_BLOCK_SIZE];
  memset(bbitmap, 0, TEST_BLOCK_SIZE);
  /* Mark bits 0–8 (blocks 1–9) as allocated */
  bbitmap[0] = 0xFF; /* bits 0-7 */
  bbitmap[1] = 0x01; /* bit 8 */
  write_block(test_image, BLK_BBITMAP, bbitmap, TEST_BLOCK_SIZE);

  /* ── Inode Bitmap (block 4) ──
   * Mark inodes 1 and 2 as used (bits 0 and 1).
   */
  uint8_t ibitmap[TEST_BLOCK_SIZE];
  memset(ibitmap, 0, TEST_BLOCK_SIZE);
  ibitmap[0] = 0x03; /* bits 0,1 = inodes 1,2 */
  write_block(test_image, BLK_IBITMAP, ibitmap, TEST_BLOCK_SIZE);

  /* ── Inode Table (blocks 5–8) ──
   * Only inode 2 (root directory) needs setup.
   * Inode numbering: inode N is at index (N-1) in the table.
   */
  struct ext2_inode root_inode;
  memset(&root_inode, 0, sizeof(root_inode));
  root_inode.i_mode = EXT2_S_IFDIR | 0755;
  root_inode.i_size = TEST_BLOCK_SIZE; /* one block of dir data */
  root_inode.i_links_count =
      2; /* '.' + itself from parent (root is its own parent) */
  root_inode.i_blocks = TEST_BLOCK_SIZE / 512;
  root_inode.i_block[0] = BLK_ROOTDIR;

  /* Write inode 2 at offset (2-1)*128 = 128 bytes into inode table */
  memcpy(test_image + BLK_ITABLE * TEST_BLOCK_SIZE + 128, &root_inode,
         sizeof(root_inode));

  /* ── Root Directory Data (block 9) ──
   * Contains '.' and '..' entries, both pointing to inode 2.
   */
  uint8_t dirblock[TEST_BLOCK_SIZE];
  memset(dirblock, 0, TEST_BLOCK_SIZE);

  struct ext2_dir_entry *dot = (struct ext2_dir_entry *)dirblock;
  dot->inode = EXT2_ROOT_INO;
  dot->rec_len = 12;
  dot->name_len = 1;
  dot->file_type = EXT2_FT_DIR;
  dot->name[0] = '.';

  struct ext2_dir_entry *dotdot = (struct ext2_dir_entry *)(dirblock + 12);
  dotdot->inode = EXT2_ROOT_INO;
  dotdot->rec_len = (uint16_t)(TEST_BLOCK_SIZE - 12);
  dotdot->name_len = 2;
  dotdot->file_type = EXT2_FT_DIR;
  dotdot->name[0] = '.';
  dotdot->name[1] = '.';

  write_block(test_image, BLK_ROOTDIR, dirblock, TEST_BLOCK_SIZE);

  /* ── Wrap in ramdisk and mount ── */
  struct blkdev *dev =
      ramdisk_create(test_image, TEST_IMAGE_SIZE, TEST_BLOCK_SIZE);
  if (!dev)
    return NULL;

  struct ext2_fs *fs = ext2_mount(dev);
  return fs;
}

void ext2_test_cleanup(struct ext2_fs *fs) {
  if (fs)
    ext2_unmount(fs);
}
