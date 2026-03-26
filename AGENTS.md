# DICRON — AI Agent Guide

Rules and conventions for AI agents working on the DICRON kernel.

---

## Code Style

- **Standard:** C11 (`-std=gnu11`)
- **Compiler flags:** `-Wall -Wextra -Werror -Wconversion -Wsign-conversion`
- **Indentation:** Tabs, not spaces
- **Braces:** K&R style — opening brace on same line
- **Include guards:** `#ifndef _DICRON_SECTION_NAME_H`
- **Pointer style:** `void *ptr` (space before `*`, not after)
- **Casts:** Always explicit. The compiler enforces `-Wconversion`.
- **Struct init:** Use `memset` or `kzalloc`, not C99 designated initializers for large structs
- **Logging:** Use `klog(KLOG_ERR|WARN|INFO|DEBUG, fmt, ...)` — never `kio_printf` for kernel messages
- **Error returns:** Negative errno values (`-EINVAL`, `-ENOMEM`, `-EIO`) or `-1` for simple failures, `0` for success
- **NULL checks:** Always validate pointer arguments at function entry

Example driver function:
```c
int my_driver_read(struct blkdev *dev, uint64_t block, void *buf)
{
	if (!dev || !buf)
		return -EINVAL;

	/* implementation */
	return 0;
}
```

---

## Kernel APIs

### Memory
| Function | Description |
|---|---|
| `kmalloc(size)` | Allocate kernel heap memory |
| `kzalloc(size)` | Allocate zeroed kernel heap memory |
| `kfree(ptr)` | Free kernel heap memory |
| `krealloc(ptr, new_size)` | Resize allocation |

### Logging
| Level | Macro | Use for |
|---|---|---|
| `KLOG_ERR` | Errors | Unrecoverable failures |
| `KLOG_WARN` | Warnings | Recoverable issues |
| `KLOG_INFO` | Info | Boot messages, driver init |
| `KLOG_DEBUG` | Debug | Verbose tracing |

### Block Devices
```c
struct blkdev {
    void *private;
    size_t block_size;
    uint64_t total_blocks;
    int (*read)(struct blkdev *dev, uint64_t block, void *buf);
    int (*write)(struct blkdev *dev, uint64_t block, const void *buf);
};
```
Create with `kzalloc(sizeof(struct blkdev))`, set fields, done.

### Character Devices
```c
struct kdev {
    const char *name;
    unsigned int minor;
    int type;           /* KDEV_CHAR or KDEV_BLOCK */
    struct kdev_ops *ops;
    void *priv;
};
```
Register with `drv_register(minor, dev)`. Max 64 devices (`DRV_MAX`).

### Filesystem (VFS)
- `vfs_namei(path)` — resolve path to inode
- `vfs_mkdir(path, mode)` — create directory
- `vfs_create(path, mode)` — create file
- `inode_alloc()` — allocate a VFS inode
- Implement `struct inode_operations` for dirs, `struct file_operations` for files

### Synchronization
- `spinlock_t lock = SPINLOCK_INIT;`
- `spin_lock(&lock)` / `spin_unlock(&lock)` — when interrupts already disabled
- `spin_lock_irqsave(&lock)` / `spin_unlock_irqrestore(&lock, flags)` — from normal context

### I/O Ports
- `inb(port)`, `outb(port, val)`, `inw(port)`, `outw(port, val)`
- Located in `arch/x86_64/io.h`

### Time
- `rtc_unix_time()` — returns `uint64_t` seconds since Unix epoch
- `rtc_read(struct rtc_time *t)` — read raw RTC fields

### Error Codes (`<dicron/errno.h>`)
`EPERM(1)`, `ENOENT(2)`, `EIO(5)`, `ENOMEM(12)`, `EBUSY(16)`, `EEXIST(17)`, `ENODEV(19)`, `EINVAL(22)`, `ERANGE(34)`, `ENOSYS(38)`

---

## Testing

Tests use the `ktest` harness. Two categories:

- **`KTEST_CAT_BOOT`** — run before scheduler starts
- **`KTEST_CAT_POST`** — run after scheduler is live

### Writing a test
```c
#include "ktest.h"

KTEST_REGISTER(test_my_thing, "My: description", KTEST_CAT_BOOT)
static void test_my_thing(void)
{
	KTEST_BEGIN("My: description");

	KTEST_EQ(actual, expected, "what this checks");
	KTEST_NOT_NULL(ptr, "ptr should be allocated");
	KTEST(expr, "boolean check");
}
```

### Available assertions
`KTEST(expr, name)`, `KTEST_EQ`, `KTEST_NE`, `KTEST_GT`, `KTEST_GE`, `KTEST_LT`, `KTEST_NOT_NULL`, `KTEST_NULL`, `KTEST_TRUE`, `KTEST_FALSE`

### Test file naming
- Boot tests: `kernel/src/tests/test_<subsystem>_<thing>.c`
- Post tests: `kernel/src/tests/post/test_post_<thing>.c`

### Enabling tests
Each test category has a `CONFIG_TESTS_<NAME>` Kconfig option. Add new test files to the appropriate `ifdef` block in `GNUmakefile`.

---

## Driver Subsystem (DQSF)

DICRON uses a **Driver Quality SubFolder** system. See `driversub.md` for full rules.

| Folder | Quality | Kconfig default |
|---|---|---|
| `kernel/src/drivers/` | **Stable** — production, well-tested | Enabled |
| `kernel/src/drivers/new/` | **New** — functional but subject to change | User chooses |
| `kernel/src/drivers/staged/` | **Staged** — unstable, stubbed, or experimental | Disabled |

### Key rules for agents
- New drivers go in `drivers/new/<name>/` first
- Never put untested code directly in `drivers/`
- Each driver needs: a Kconfig entry, Makefile wiring, and at least basic tests
- Graduation from `new/` to `drivers/` requires a formal move (update Kconfig, Makefile, includes)

---

## Kconfig

- Config symbols: `CONFIG_<NAME>` (all caps, underscores)
- Driver configs go in `kernel/Kconfig.drivers`
- Test configs go in `kernel/Kconfig.tests`
- After adding a Kconfig option, run `make oldconfig` to regenerate `.config` and `autoconf.h`
- Guard code with `#ifdef CONFIG_<NAME>` and `#include <generated/autoconf.h>`

---

## Build System

- `make` — build kernel
- `make iso` — build bootable ISO
- `make run` — build + run in QEMU
- `make menuconfig` — configure kernel options
- `make clean` — remove build artifacts

All source inclusion is in `GNUmakefile` via `ifdef CONFIG_*` blocks. When adding a new driver or test file, add it to the appropriate conditional block.
