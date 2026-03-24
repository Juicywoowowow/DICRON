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
	
	if (!root_inode->i_op || !root_inode->i_op->lookup)
		return NULL;
		
	const char *slash = my_strchr(path, '/');
	char comp[64];
	size_t len = slash ? (size_t)(slash - path) : strlen(path);
	if (len >= sizeof(comp)) len = sizeof(comp) - 1;
	memcpy(comp, path, len);
	comp[len] = '\0';
	
	struct inode *child = root_inode->i_op->lookup(root_inode, comp);
	if (!child) return NULL;
	
	if (slash) {
		const char *rest = slash + 1;
		while (*rest == '/') rest++;
		if (*rest != '\0') return NULL; /* flat root only for now */
	}
	
	return child;
}
