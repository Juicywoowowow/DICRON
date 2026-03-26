DEVFS (Device Filesystem)
=========================

1. Overview
-----------

DEVFS provides device files in /dev for accessing kernel devices.
It creates special files that allow userspace programs to interact
with hardware and kernel services.

2. Initialization
----------------

  void devfs_init(void)
      - Initialize DEVFS and register with VFS

3. Available Devices
--------------------

DEVFS typically provides:
  - /dev/null  - Discard device
  - /dev/zero  - Zero-filled read device
  - /dev/console - System console

4. Device Operations
------------------

Each device implements file_operations:
  - read: Read from device
  - write: Write to device
  - ioctl: Device-specific control

5. Console Access
----------------

  struct file *devfs_get_console(void)
      - Get file pointer for console output

6. Implementation Details
------------------------

DEVFS is a simple filesystem that does not use a block device.
Instead, it provides character device semantics for each file.
