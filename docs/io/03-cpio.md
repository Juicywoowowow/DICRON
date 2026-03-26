CPIO Archive Format
====================

1. Overview
-----------

CPIO is an archive format used for the initial ramdisk (initrd). DICRON
supports the SVR4 CPIO format for extracting the initrd provided by the
bootloader.

2. Format
---------

Each entry has a header followed by filename and file data:
  [Header (26 16-bit words)] [Filename] [NUL pad] [File Data]

The header contains:
  - c_magic: 0x303707 or 0x303737 (octal or newc)
  - c_dev, c_ino: Device/inode numbers
  - c_mode: File mode and permissions
  - c_uid, c_gid: User/group IDs
  - c_nlink: Number of links
  - c_mtime: Modification time
  - c_size: File size in bytes
  - c_maj, c_min: Major/minor device numbers
  - c_rmaj, c_rmin: For special files
  - c_namesize: Length of filename including NUL
  - c_check: Checksum (unused, set to 0)

3. Entry Types
--------------

  Regular file (S_IFREG)
  Directory (S_IFDIR)
  Symbolic link (S_IFLNK)
  Character device (S_IFCHR)
  Block device (S_IFBLK)
  FIFO (S_IFIFO)

4. API
-----

  int cpio_extract_all(void *data, size_t size)
      - Extract entire cpio archive to RAMFS
      - Returns 0 on success

  int cpio_parse_entry(const char *data, size_t size,
                       struct cpio_entry *out, size_t *header_size)
      - Parse single entry from buffer
      - Returns 0 if found valid entry

5. Trailing Marker
------------------

CPIO archives end with a special entry with filename "TRAILER!!!"
that indicates end of archive.

6. Files
-------

  cpio_parse.c   - Parse cpio entry format
  cpio_utils.c   - Utility functions
  cpio_extract.c - Extraction to RAMFS
