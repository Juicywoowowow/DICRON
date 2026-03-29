#include "ext2.h"
#include <dicron/fs.h>
#include <dicron/mem.h>
#include <dicron/log.h>
#include "lib/string.h"

/* Per-inode private data linking back to ext2 */
struct ext2_vfs_inode_info {
	struct ext2_fs *fs;
	uint32_t ino;
	struct ext2_inode raw;
	struct dentry *dentry;  /* for directory child lookup */
};

static struct inode_operations ext2_dir_ops;

static long ext2_vfs_read(struct file *f, char *buf, size_t count)
{
	struct ext2_vfs_inode_info *info = f->f_inode->i_private;
	long ret = ext2_file_read(info->fs, &info->raw, buf,
				  (size_t)f->f_pos, count);
	if (ret > 0)
		f->f_pos += (uint64_t)ret;
	return ret;
}

static struct file_operations ext2_file_ops = {
	.read = ext2_vfs_read,
	.write = NULL,  /* Phase F6 */
	.ioctl = NULL,
	.release = NULL,
};

/* Allocate a VFS inode backed by an ext2 inode */
static struct inode *ext2_vfs_make_inode(struct ext2_fs *fs, uint32_t ino,
					 struct dentry *parent_dentry)
{
	struct ext2_inode raw;
	if (ext2_read_inode(fs, ino, &raw) != 0)
		return NULL;

	struct inode *vi = inode_alloc();
	if (!vi) return NULL;

	vi->mode = raw.i_mode;
	vi->uid = raw.i_uid;
	vi->gid = raw.i_gid;
	vi->size = raw.i_size;

	struct ext2_vfs_inode_info *info = kzalloc(sizeof(*info));
	if (!info) { kfree(vi); return NULL; }

	info->fs = fs;
	info->ino = ino;
	info->raw = raw;
	info->dentry = NULL;
	vi->i_private = info;

	if ((raw.i_mode & EXT2_S_IFMT) == EXT2_S_IFDIR) {
		vi->i_op = &ext2_dir_ops;
	} else {
		vi->f_op = &ext2_file_ops;
	}

	(void)parent_dentry;
	return vi;
}

static struct inode *ext2_vfs_lookup(struct inode *dir, const char *name)
{
	struct ext2_vfs_inode_info *info = dir->i_private;
	uint32_t ino = ext2_dir_lookup(info->fs, &info->raw, name);
	if (ino == 0)
		return NULL;
	return ext2_vfs_make_inode(info->fs, ino, NULL);
}

static struct inode_operations ext2_dir_ops = {
	.lookup = ext2_vfs_lookup,
	.create = NULL,  /* Phase F6 */
	.mkdir = NULL,   /* Phase F6 */
};

int ext2_vfs_mount(struct ext2_fs *fs, const char *mountpoint)
{
	struct inode *root = ext2_vfs_make_inode(fs, EXT2_ROOT_INO, NULL);
	if (!root) {
		klog(KLOG_ERR, "ext2: failed to read root inode\n");
		return -1;
	}

	/*
	 * For now, create a dentry under the VFS root.
	 * e.g., mount at "/disk" — create a dir inode at that path,
	 * then hook ext2 lookup into it.
	 */
	if (mountpoint) {
		/* Create the mount point directory, then swap its ops */
		struct inode *mp = vfs_namei(mountpoint);
		if (!mp) {
			vfs_mkdir(mountpoint, 0755);
			mp = vfs_namei(mountpoint);
		}
		if (!mp) {
			klog(KLOG_ERR, "ext2: can't create mountpoint '%s'\n",
			     mountpoint);
			return -1;
		}
		/* Replace the ramfs dir's ops with ext2 ops */
		mp->i_op = &ext2_dir_ops;
		mp->i_private = root->i_private;
		mp->mode = root->mode;
		mp->size = root->size;
		klog(KLOG_DEBUG, "ext2: mounted at %s\n", mountpoint);
	}

	return 0;
}
