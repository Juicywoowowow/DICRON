Virtual File System (VFS)
=========================

1. Overview
-----------

The Virtual File System provides a uniform interface for file
operations regardless of the underlying filesystem. It allows the
kernel to access files through a common API while supporting
multiple filesystem implementations.

2. Core Data Structures
-----------------------

Inode (struct inode):
  - ino: Unique inode number
  - mode: File type and permissions
  - uid, gid: Owner user and group
  - size: File size in bytes
  - i_op: Pointer to inode operations
  - f_op: Pointer to file operations
  - i_private: Filesystem-specific private data

Dentry (struct dentry):
  - name: Filename (up to 63 characters)
  - d_inode: Associated inode
  - d_parent: Parent dentry
  - d_siblings: Linked list of siblings
  - d_children: Linked list of children

File (struct file):
  - f_inode: Associated inode
  - f_dentry: Associated dentry
  - f_op: File operations
  - f_flags: Open flags (O_RDONLY, etc.)
  - f_mode: File mode
  - f_pos: Current file position
  - f_count: Reference count
  - f_private: File-specific private data

3. Operations
------------

File Operations (file_operations):
  - read: Read from file
  - write: Write to file
  - ioctl: Device control
  - release: Close file

Inode Operations (inode_operations):
  - lookup: Look up a child by name
  - create: Create a regular file
  - mkdir: Create a directory

4. API
-----

File descriptor management:
  struct file *file_alloc(void)
      - Allocate a new file structure

  void file_get(struct file *f)
      - Increment reference count

  void file_put(struct file *f)
      - Decrement reference count, free if zero

  struct file *fd_get(int fd)
      - Get file by file descriptor number

  int fd_install(struct file *f)
      - Install file in file descriptor table

  int fd_close(int fd)
      - Close a file descriptor

Inode/Dentry management:
  struct inode *inode_alloc(void)
      - Allocate a new inode

  struct dentry *dentry_alloc(const char *name, struct dentry *parent)
      - Allocate a new dentry

  struct inode *vfs_namei(const char *path)
      - Resolve a path to an inode

  int vfs_mount_root(struct inode *root_dir)
      - Set the root filesystem

VFS operations:
  int vfs_mkdir(const char *path, int mode)
      - Create a directory

  int vfs_create(const char *path, int mode)
      - Create a regular file

5. Path Resolution
-----------------

vfs_namei() performs path resolution:
  1. Skip leading slashes
  2. For each path component:
     a. Call lookup on current inode
     b. Advance to child inode
  3. Return final inode

Path components are limited to 63 characters.

6. File Descriptor Table
------------------------

Each process has a file descriptor table (fds array in process struct)
with MAX_FDS entries (64). File descriptors are numeric indices into
this table. When a file is opened, the smallest available fd is used.

7. Reference Counting
--------------------

Files, inodes, and dentries use reference counting:
  - file_get/file_put for file structures
  - inode reference in dentry
  - dentry reference in parent/children

When reference count drops to zero, the structure is freed.
