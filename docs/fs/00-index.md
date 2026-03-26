Filesystem Subsystem
====================

1. Overview
-----------

DICRON implements a layered filesystem architecture:

  +-------------------+
  | VFS (Virtual FS) | - Common interface
  +-------------------+
         |
  +------+--------+--------+
  |      |        |        |
 ramfs devfs   ext2   (more)

2. Components
------------

  VFS:    Virtual File System - common API
  ramfs:  RAM Filesystem - initrd storage
  devfs:  Device Filesystem - /dev special files
  ext2:   Second Extended FS - disk filesystem
  cpio:   Archive format - initrd extraction

3. File Descriptor Interface
-----------------------------

Each process has file descriptors:
  - MAX_FDS = 64 per process
  - fd 0: stdin (usually /dev/console)
  - fd 1: stdout (usually /dev/console)
  - fd 2: stderr (usually /dev/console)

4. VFS Operations
-----------------

  File operations:
  - read, write, ioctl, release

  Inode operations:
  - lookup, create, mkdir

5. File Types
------------

  S_IFREG  - Regular file
  S_IFDIR  - Directory
  S_IFLNK  - Symbolic link
  S_IFCHR  - Character device
  S_IFBLK  - Block device
  S_IFIFO  - FIFO/pipe

6. Open Flags
------------

  O_RDONLY  - Read only
  O_WRONLY  - Write only
  O_RDWR    - Read/write
  O_CREAT   - Create if doesn't exist
  O_EXCL    - Fail if exists (with O_CREAT)
  O_TRUNC   - Truncate to zero length
  O_APPEND  - Append to end of file

7. Mount Points
----------------

The VFS supports mounting filesystems:
  - Root filesystem: mounted at / (vfs_mount_root)
  - Additional mounts not yet implemented
