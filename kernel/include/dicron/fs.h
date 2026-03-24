#ifndef _DICRON_FS_H
#define _DICRON_FS_H

#include <stdint.h>
#include <stddef.h>

#define MAX_FDS 64

#define O_RDONLY    00000000U
#define O_WRONLY    00000001U
#define O_RDWR      00000002U
#define O_CREAT     00000100U
#define O_EXCL      00000200U
#define O_TRUNC     00001000U
#define O_APPEND    00002000U

#define S_IFMT      0170000U
#define S_IFSOCK    0140000U
#define S_IFLNK     0120000U
#define S_IFREG     0100000U
#define S_IFBLK     0060000U
#define S_IFDIR     0040000U
#define S_IFCHR     0020000U
#define S_IFIFO     0010000U

#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)
#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)

struct inode;
struct file;
struct dentry;

struct file_operations {
	long (*read)(struct file *f, char *buf, size_t count);
	long (*write)(struct file *f, const char *buf, size_t count);
	long (*ioctl)(struct file *f, unsigned int cmd, unsigned long arg);
	int (*release)(struct inode *i, struct file *f);
};

struct inode_operations {
	struct inode *(*lookup)(struct inode *dir, const char *name);
	int (*create)(struct inode *dir, const char *name, int mode);
	int (*mkdir)(struct inode *dir, const char *name, int mode);
};

struct inode {
	uint32_t ino;
	uint32_t mode;
	uint32_t uid;
	uint32_t gid;
	uint64_t size;
	
	const struct inode_operations *i_op;
	const struct file_operations *f_op;
	
	void *i_private;
};

struct dentry {
	char name[64];
	struct inode *d_inode;
	struct dentry *d_parent;
	struct dentry *d_siblings;
	struct dentry *d_children;
};

struct file {
	struct inode *f_inode;
	struct dentry *f_dentry;
	const struct file_operations *f_op;
	uint32_t f_flags;
	uint32_t f_mode;
	uint64_t f_pos;
	int f_count;
	void *f_private;
};

struct file *file_alloc(void);
void file_get(struct file *f);
void file_put(struct file *f);

struct file *fd_get(int fd);
int fd_install(struct file *f);
int fd_close(int fd);
void process_fd_init(void *process_ptr);
void process_fd_cleanup(void *process_ptr);

void vfs_init(void);
struct inode *inode_alloc(void);
struct dentry *dentry_alloc(const char *name, struct dentry *parent);

struct inode *vfs_namei(const char *path);
int vfs_mount_root(struct inode *root_dir);

void devfs_init(void);
struct file *devfs_get_console(void);

void ramfs_init(void);

#endif
