#ifndef _DICRON_DRIVERS_EXT2_H
#define _DICRON_DRIVERS_EXT2_H

#include <stdint.h>
#include <stddef.h>
#include <dicron/blkdev.h>

/* ── Magic ── */
#define EXT2_MAGIC          0xEF53

/* ── Special inodes ── */
#define EXT2_ROOT_INO       2

/* ── Inode mode flags ── */
#define EXT2_S_IFMT         0xF000
#define EXT2_S_IFREG        0x8000
#define EXT2_S_IFDIR        0x4000
#define EXT2_S_IFLNK        0xA000

/* ── File type in dir entry ── */
#define EXT2_FT_UNKNOWN     0
#define EXT2_FT_REG_FILE    1
#define EXT2_FT_DIR         2
#define EXT2_FT_SYMLINK     7

/* ── Block pointers ── */
#define EXT2_NDIR_BLOCKS    12
#define EXT2_IND_BLOCK      12
#define EXT2_DIND_BLOCK     13
#define EXT2_TIND_BLOCK     14
#define EXT2_N_BLOCKS       15

/* ── Superblock (offset 1024, 1024 bytes) ── */
struct ext2_superblock {
	uint32_t s_inodes_count;
	uint32_t s_blocks_count;
	uint32_t s_r_blocks_count;
	uint32_t s_free_blocks_count;
	uint32_t s_free_inodes_count;
	uint32_t s_first_data_block;
	uint32_t s_log_block_size;
	uint32_t s_log_frag_size;
	uint32_t s_blocks_per_group;
	uint32_t s_frags_per_group;
	uint32_t s_inodes_per_group;
	uint32_t s_mtime;
	uint32_t s_wtime;
	uint16_t s_mnt_count;
	uint16_t s_max_mnt_count;
	uint16_t s_magic;
	uint16_t s_state;
	uint16_t s_errors;
	uint16_t s_minor_rev_level;
	uint32_t s_lastcheck;
	uint32_t s_checkinterval;
	uint32_t s_creator_os;
	uint32_t s_rev_level;
	uint16_t s_def_resuid;
	uint16_t s_def_resgid;
	/* Rev 1 fields */
	uint32_t s_first_ino;
	uint16_t s_inode_size;
	uint16_t s_block_group_nr;
	uint32_t s_feature_compat;
	uint32_t s_feature_incompat;
	uint32_t s_feature_ro_compat;
	uint8_t  s_uuid[16];
	char     s_volume_name[16];
	char     s_last_mounted[64];
	uint32_t s_algo_bitmap;
	/* Padding to 1024 bytes handled by reading fixed size */
} __attribute__((packed));

/* ── Block Group Descriptor (32 bytes) ── */
struct ext2_group_desc {
	uint32_t bg_block_bitmap;
	uint32_t bg_inode_bitmap;
	uint32_t bg_inode_table;
	uint16_t bg_free_blocks_count;
	uint16_t bg_free_inodes_count;
	uint16_t bg_used_dirs_count;
	uint16_t bg_pad;
	uint8_t  bg_reserved[12];
} __attribute__((packed));

/* ── Inode (128 bytes for rev 0) ── */
struct ext2_inode {
	uint16_t i_mode;
	uint16_t i_uid;
	uint32_t i_size;
	uint32_t i_atime;
	uint32_t i_ctime;
	uint32_t i_mtime;
	uint32_t i_dtime;
	uint16_t i_gid;
	uint16_t i_links_count;
	uint32_t i_blocks;       /* 512-byte sectors, not fs blocks */
	uint32_t i_flags;
	uint32_t i_osd1;
	uint32_t i_block[EXT2_N_BLOCKS];
	uint32_t i_generation;
	uint32_t i_file_acl;
	uint32_t i_dir_acl;      /* i_size_high for regular files in rev 1 */
	uint32_t i_faddr;
	uint8_t  i_osd2[12];
} __attribute__((packed));

/* ── Directory Entry ── */
struct ext2_dir_entry {
	uint32_t inode;
	uint16_t rec_len;
	uint8_t  name_len;
	uint8_t  file_type;
	char     name[];          /* variable length, NOT null-terminated */
} __attribute__((packed));

/* ── Driver state ── */
struct ext2_fs {
	struct blkdev *dev;
	struct ext2_superblock sb;
	struct ext2_group_desc *group_descs;
	uint32_t block_size;
	uint32_t groups_count;
	uint32_t inodes_per_block;
	uint32_t inode_size;
	uint32_t addrs_per_block; /* block_size / 4 — entries in indirect block */
};

/* ── API ── */

/* ext2_super.c */
struct ext2_fs *ext2_mount(struct blkdev *dev);
void ext2_unmount(struct ext2_fs *fs);

/* ext2_block.c */
int ext2_read_block(struct ext2_fs *fs, uint32_t block_nr, void *buf);
int ext2_write_block(struct ext2_fs *fs, uint32_t block_nr, const void *buf);

/* ext2_inode.c */
int ext2_read_inode(struct ext2_fs *fs, uint32_t ino, struct ext2_inode *out);
uint32_t ext2_inode_block_nr(struct ext2_fs *fs, struct ext2_inode *inode,
			     uint32_t file_block);

/* ext2_dir.c */
uint32_t ext2_dir_lookup(struct ext2_fs *fs, struct ext2_inode *dir,
			 const char *name);

/* ext2_file.c */
long ext2_file_read(struct ext2_fs *fs, struct ext2_inode *inode,
		    void *buf, size_t offset, size_t count);

/* ext2_vfs.c */
int ext2_vfs_mount(struct ext2_fs *fs, const char *mountpoint);

#endif
