#include <dicron/fs.h>
#include <dicron/mem.h>
#include "lib/string.h"

struct ramfs_inode_info {
	char *data;
	size_t capacity;
};

static long ramfs_read(struct file *f, char *buf, size_t count)
{
	struct inode *i = f->f_inode;
	struct ramfs_inode_info *info = i->i_private;
	
	if (f->f_pos >= i->size) return 0;
	
	size_t available = i->size - f->f_pos;
	if (count > available) count = available;
	
	memcpy(buf, info->data + f->f_pos, count);
	f->f_pos += count;
	return (long)count;
}

static long ramfs_write(struct file *f, const char *buf, size_t count)
{
	struct inode *i = f->f_inode;
	struct ramfs_inode_info *info = i->i_private;
	
	if (f->f_pos + count > info->capacity) {
		size_t new_cap = info->capacity == 0 ? 4096 : info->capacity * 2;
		while (new_cap < f->f_pos + count) new_cap *= 2;
		
		char *new_data = kzalloc(new_cap);
		if (!new_data) return -1;
		
		if (info->data) {
			memcpy(new_data, info->data, i->size);
			kfree(info->data);
		}
		info->data = new_data;
		info->capacity = new_cap;
	}
	
	memcpy(info->data + f->f_pos, buf, count);
	f->f_pos += count;
	if (f->f_pos > i->size) i->size = f->f_pos;
	
	return (long)count;
}

static struct file_operations ramfs_file_operations = {
	.read = ramfs_read,
	.write = ramfs_write,
	.ioctl = NULL,
	.release = NULL,
};

static struct dentry *ramfs_root_dentry;

static struct inode_operations ramfs_dir_operations;

static struct dentry *ramfs_find_dentry(struct inode *dir, const char *name)
{
	struct dentry *parent_d = ramfs_root_dentry;

	/* Walk dentry tree to find the dentry whose inode matches dir */
	if (dir->i_private) {
		parent_d = (struct dentry *)dir->i_private;
	}

	struct dentry *d = parent_d->d_children;
	size_t nlen = strlen(name);
	while (d) {
		size_t dlen = strlen(d->name);
		if (nlen == dlen && memcmp(name, d->name, nlen) == 0)
			return d;
		d = d->d_siblings;
	}
	return NULL;
}

static struct inode *ramfs_lookup(struct inode *dir, const char *name)
{
	struct dentry *d = ramfs_find_dentry(dir, name);
	return d ? d->d_inode : NULL;
}

static struct dentry *ramfs_get_dentry(struct inode *dir)
{
	if (dir->i_private)
		return (struct dentry *)dir->i_private;
	return ramfs_root_dentry;
}

static int ramfs_create(struct inode *dir, const char *name, int mode)
{
	if (ramfs_lookup(dir, name)) return -1;

	struct inode *i = inode_alloc();
	if (!i) return -1;

	i->mode = S_IFREG | (uint32_t)mode;
	i->f_op = &ramfs_file_operations;

	struct ramfs_inode_info *info = kzalloc(sizeof(struct ramfs_inode_info));
	if (!info) { kfree(i); return -1; }
	i->i_private = info;

	struct dentry *d = dentry_alloc(name, ramfs_get_dentry(dir));
	if (!d) { kfree(info); kfree(i); return -1; }

	d->d_inode = i;

	return 0;
}

static int ramfs_mkdir(struct inode *dir, const char *name, int mode)
{
	if (ramfs_lookup(dir, name)) return -1;

	struct inode *i = inode_alloc();
	if (!i) return -1;

	i->mode = S_IFDIR | (uint32_t)mode;
	i->i_op = &ramfs_dir_operations;

	struct dentry *d = dentry_alloc(name, ramfs_get_dentry(dir));
	if (!d) { kfree(i); return -1; }

	d->d_inode = i;
	i->i_private = d; /* dir inodes point to their dentry for child lookup */

	return 0;
}

static struct inode_operations ramfs_dir_operations = {
	.lookup = ramfs_lookup,
	.create = ramfs_create,
	.mkdir = ramfs_mkdir,
};

void ramfs_init(void)
{
	struct inode *root = inode_alloc();
	if (!root) return;
	root->mode = S_IFDIR | 0755;
	root->i_op = &ramfs_dir_operations;

	ramfs_root_dentry = dentry_alloc("/", NULL);
	ramfs_root_dentry->d_inode = root;
	root->i_private = ramfs_root_dentry;

	vfs_mount_root(root);
}
