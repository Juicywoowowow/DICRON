#include <dicron/fs.h>
#include <dicron/mem.h>
#include <dicron/log.h>
#include "lib/string.h"

static struct inode *root_inode = NULL;
static uint32_t next_ino = 1;

static const char *my_strchr(const char *s, int c) {
	while (*s) {
		if (*s == (char)c) return s;
		s++;
	}
	return NULL;
}

void vfs_init(void)
{
	klog(KLOG_INFO, "VFS initialized.\n");
}

struct inode *inode_alloc(void)
{
	struct inode *i = kzalloc(sizeof(struct inode));
	if (i)
		i->ino = next_ino++;
	return i;
}

struct dentry *dentry_alloc(const char *name, struct dentry *parent)
{
	struct dentry *d = kzalloc(sizeof(struct dentry));
	if (!d) return NULL;
	
	size_t len = strlen(name);
	if (len >= sizeof(d->name)) len = sizeof(d->name) - 1;
	memcpy(d->name, name, len);
	d->name[len] = '\0';
	
	d->d_parent = parent ? parent : d;
	
	if (parent) {
		d->d_siblings = parent->d_children;
		parent->d_children = d;
	}
	
	return d;
}

int vfs_mount_root(struct inode *root_dir)
{
	if (!root_dir || !S_ISDIR(root_dir->mode)) return -1;
	root_inode = root_dir;
	klog(KLOG_INFO, "VFS: Mounted root filesystem.\n");
	return 0;
}

struct inode *vfs_namei(const char *path)
{
	if (!root_inode) return NULL;

	while (*path == '/') path++;
	if (*path == '\0') return root_inode;

	struct inode *cur = root_inode;

	while (*path) {
		if (!cur->i_op || !cur->i_op->lookup)
			return NULL;

		const char *slash = my_strchr(path, '/');
		char comp[64];
		size_t len = slash ? (size_t)(slash - path) : strlen(path);
		if (len >= sizeof(comp)) len = sizeof(comp) - 1;
		memcpy(comp, path, len);
		comp[len] = '\0';

		cur = cur->i_op->lookup(cur, comp);
		if (!cur) return NULL;

		if (slash) {
			path = slash + 1;
			while (*path == '/') path++;
		} else {
			break;
		}
	}

	return cur;
}

static struct inode *vfs_parent_and_name(const char *path, const char **out_name)
{
	if (!root_inode || !path || *path != '/') return NULL;

	while (*path == '/') path++;
	if (*path == '\0') return NULL;

	struct inode *cur = root_inode;

	for (;;) {
		const char *slash = my_strchr(path, '/');
		if (!slash) {
			*out_name = path;
			return cur;
		}

		char comp[64];
		size_t len = (size_t)(slash - path);
		if (len >= sizeof(comp)) len = sizeof(comp) - 1;
		memcpy(comp, path, len);
		comp[len] = '\0';

		if (!cur->i_op || !cur->i_op->lookup) return NULL;
		cur = cur->i_op->lookup(cur, comp);
		if (!cur) return NULL;

		path = slash + 1;
		while (*path == '/') path++;
		if (*path == '\0') return NULL;
	}
}

int vfs_mkdir(const char *path, int mode)
{
	const char *name = NULL;
	struct inode *parent = vfs_parent_and_name(path, &name);
	if (!parent || !name) return -1;
	if (!parent->i_op || !parent->i_op->mkdir) return -1;
	return parent->i_op->mkdir(parent, name, mode);
}

int vfs_create(const char *path, int mode)
{
	const char *name = NULL;
	struct inode *parent = vfs_parent_and_name(path, &name);
	if (!parent || !name) return -1;
	if (!parent->i_op || !parent->i_op->create) return -1;
	return parent->i_op->create(parent, name, mode);
}
