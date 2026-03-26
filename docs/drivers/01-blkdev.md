Block Device Layer
==================

1. Overview
-----------

The block device layer provides a uniform interface for block-oriented
storage devices. It abstracts away the details of different storage
hardware (ATA disks, ramdisk, etc.) behind a common API.

2. Block Device Structure
--------------------------

  struct blkdev {
      void *private;           - Driver-specific data
      size_t block_size;       - Size of each block (usually 512)
      uint64_t total_blocks;   - Total number of blocks
      int (*read)();           - Read blocks
      int (*write)();         - Write blocks
  };

3. API
-----

  struct blkdev *ramdisk_create(void *data, size_t size, size_t block_size)
      - Create a memory-backed block device
      - Returns blkdev pointer

4. Usage
--------

Filesystems use block devices for persistent storage:
  - EXT2 mounts on blkdev
  - Read/write operations go through blkdev abstraction

5. Implementation
-----------------

Block device drivers implement:
  - read: Read sector(s) from device
  - write: Write sector(s) to device
  - Driver-specific initialization
