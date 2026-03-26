RAMFS (RAM Filesystem)
======================

1. Overview
-----------

RAMFS is an in-memory filesystem that stores files in RAM. It is used
as the initial root filesystem (initrd) populated from a cpio archive
provided by the bootloader.

2. Implementation
----------------

RAMFS is implemented in src/fs/ramfs.c and uses a simple tree structure:
  - Each file/directory is represented by an inode
  - Directory contents stored as a linked list of entries
  - File data stored inline in inode

3. API
-----

  void ramfs_init(void)
      - Initialize RAMFS and register with VFS

4. Features
-----------

  - Create files and directories
  - Read/write file data
  - Lookup by name
  - Supports regular files and directories

5. Use Cases
------------

  - Initrd filesystem: Holds /init and other boot files
  - Temporary storage for runtime data

6. Limitations
--------------

  - All data is lost on reboot
  - Limited by available RAM
  - No persistence to disk

7. CPIO Archive Support
-----------------------

RAMFS is populated from cpio archives (SVR4 format). The cpio_extract
functions in src/fs/cpio_parse.c and src/fs/cpio_extract.c handle
parsing and extracting the archive into RAMFS.
