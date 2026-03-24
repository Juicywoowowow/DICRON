# DICRON Kernel — Implementation Plan (v3)

A monolithic x86-64 kernel written from scratch in C, Linux kernel coding style. Limine handles boot (BIOS mode). Focus: clean, professional API that makes kernel extension and distro building pleasant.

---

## Kernel API Design

The public API that distro builders (and kernel extensions) use.

All public headers live in `kernel/include/dicron/`. Distro and extension code uses `#include <dicron/io.h>`, `#include <dicron/mem.h>`, etc. Internal subsystem headers are **never** included directly — they stay inside each subsystem's directory.

> **Why `dicron/` instead of `kapi/`?**
> Namespacing by project name is standard practice (Linux uses `linux/`, FreeBSD uses `sys/`). `kapi/` is generic and unclear. `dicron/` is unambiguous and avoids the stutter of `#include <kapi/kio.h>` where the `k` prefix appears twice.

### `dicron/io.h` — Kernel I/O

```c
/*
 * dicron/io.h — unified kernel I/O
 *
 * Output goes to all registered output devices (console + serial).
 * Input reads from the primary input device (keyboard).
 */
int  kio_putchar(int c);
int  kio_puts(const char *s);
int  kio_printf(const char *fmt, ...);
int  kio_getchar(void);                /* blocking read */
```

### `dicron/mem.h` — Kernel Memory

```c
/*
 * dicron/mem.h — kernel memory allocation
 *
 * Public API exposes only the heap allocator.
 * Raw physical page allocation is an internal subsystem detail
 * and is NOT exposed here.
 */
void *kmalloc(size_t size);
void *kzalloc(size_t size);           /* malloc + zero */
void  kfree(void *ptr);
```

> **Why no `pmm_alloc_page` / `pmm_free_page` here?**
> These are internal memory-subsystem functions. Exposing them in the public API breaks the abstraction — distro code should never manually manage physical pages. If a future extension needs raw pages, we add a controlled `kpage_alloc()` wrapper then.

### `dicron/log.h` — Kernel Logging

```c
/*
 * dicron/log.h — leveled kernel logging
 *
 * Output goes to serial + console. Levels filter noise.
 */
#define KLOG_EMERG   0
#define KLOG_ERR     1
#define KLOG_WARN    2
#define KLOG_INFO    3
#define KLOG_DEBUG   4

void klog(int level, const char *fmt, ...);
```

### `dicron/dev.h` — Device Model

```c
/*
 * dicron/dev.h — generic device model
 *
 * Every driver exposes a struct kdev with an ops table.
 * The driver registry provides lookup by ID.
 */
struct kdev;

struct kdev_ops {
        int     (*open)(struct kdev *dev);
        int     (*close)(struct kdev *dev);
        ssize_t (*read)(struct kdev *dev, void *buf, size_t len);
        ssize_t (*write)(struct kdev *dev, const void *buf, size_t len);
        int     (*ioctl)(struct kdev *dev, unsigned long cmd, void *arg);
};

struct kdev {
        const char      *name;
        unsigned int     id;
        struct kdev_ops *ops;
        void            *priv;          /* driver-private data */
};
```

### Distro code example

```c
/* dtest/main.c */
#include <dicron/io.h>
#include <dicron/mem.h>
#include <dicron/log.h>

void dtest_main(void)
{
        kio_printf("=== DTEST 1.0 ===\n");

        void *p = kmalloc(4096);
        if (!p) {
                klog(KLOG_ERR, "kmalloc failed\n");
                return;
        }
        kio_printf("allocated page at %p\n", p);
        kfree(p);

        kio_printf("type something: ");
        int c;
        while ((c = kio_getchar()) != '\n')
                kio_putchar(c);
        kio_putchar('\n');
}
```

---

## Project Structure

```
DICRON/
├── GNUmakefile                    # Build kernel, make ISO, run QEMU
├── limine.conf                    # Limine bootloader config (BIOS)
├── Limine/                        # Pre-built bootloader binaries
│
├── kernel/
│   ├── linker.lds
│   │
│   ├── include/
│   │   └── dicron/                # ★ Public API headers (the ONLY thing distro code includes)
│   │       ├── io.h
│   │       ├── mem.h
│   │       ├── log.h
│   │       └── dev.h
│   │
│   └── src/
│       ├── limine.h               # Protocol header (downloaded)
│       ├── main.c                 # kmain — entry point
│       │
│       ├── lib/                   # Freestanding libc helpers
│       │   ├── string.h
│       │   ├── string.c
│       │   ├── printf.h
│       │   └── printf.c           # Shared vsnprintf engine
│       │
│       ├── console/               # Framebuffer text console
│       │   ├── console.h
│       │   ├── console.c
│       │   └── font.h
│       │
│       ├── mm/                    # Memory management (internal)
│       │   ├── pmm.h
│       │   ├── pmm.c             # Physical page allocator (bitmap)
│       │   ├── vmm.h
│       │   ├── vmm.c             # Page-table manipulation
│       │   ├── kheap.h
│       │   └── kheap.c           # kmalloc / kfree
│       │
│       ├── arch/x86_64/           # CPU / arch-specific
│       │   ├── gdt.h
│       │   ├── gdt.c
│       │   ├── idt.h
│       │   ├── idt.c
│       │   ├── idt_stubs.asm
│       │   └── io.h               # outb / inb inlines
│       │
│       ├── drivers/               # Device drivers
│       │   ├── registry.h         # Driver registry — lookup by ID
│       │   ├── registry.c
│       │   ├── serial/
│       │   │   ├── com.h
│       │   │   └── com.c          # COM1 serial
│       │   └── ps2/
│       │       ├── kbd.h
│       │       └── kbd.c          # PS/2 keyboard
│       │
│       ├── io/                    # kio / klog implementations
│       │   ├── kio.c
│       │   └── klog.c
│       │
│       └── syscall/
│           └── syscall_nr.h       # Reserved numbers only
│
└── dtest/
    └── main.c                     # Basic distro test using public API
```

### Structure rationale vs. v2

| v2 problem | v3 fix |
|---|---|
| `kapi/` headers lived inside `kernel/src/` mixed with implementation | Public headers moved to `kernel/include/dicron/` — clean separation of interface and implementation, standard layout |
| Header names stuttered: `#include <kapi/kio.h>` | Now `#include <dicron/io.h>` — namespace is the project, file is the subsystem |
| `drvtb.h` / `drvtb.c` — cryptic abbreviation | Renamed to `registry.h` / `registry.c` — self-documenting |
| `dtest/1/dtest.c` — the `1/` nesting and redundant name | Flattened to `dtest/main.c` — no reason for numbered subdirectories at this stage |
| `lib/string.h / .c` notation — ambiguous (one file or two?) | Each file listed separately |
| `pmm_alloc_page`/`pmm_free_page` in public `kmem.h` | Removed from public API — stays internal to `mm/` |
| `drvutil.h / .c` listed but never referenced | Removed — add it when actually needed |

---

## Driver Registry

Flat array indexed by driver ID, O(1) lookup:

```c
/* drivers/registry.h */
#define DRV_MAX         64

/* Well-known driver IDs */
#define DRV_SERIAL      0
#define DRV_KBD         1

int          drv_register(unsigned int id, struct kdev *dev);
int          drv_unregister(unsigned int id);
struct kdev *drv_get(unsigned int id);   /* O(1) */
```

> **Naming**: `drv_register` / `drv_get` instead of `drvtb_register` / `drvtb_get`. The `tb` (table) suffix leaked implementation details into the API name. Callers don't care that it's backed by a table.

Each driver folder (e.g. `drivers/serial/com.c`) implements a `struct kdev_ops`, allocates a `struct kdev`, and calls `drv_register()` during init. The kio layer iterates registered output devices for writes.

---

## Environment Setup

```bash
wsl -d Ubuntu-22.04 -- bash -c "sudo apt-get update && sudo apt-get install -y \
    gcc nasm make xorriso mtools qemu-system-x86"
```

Download `limine.h`:
```bash
wsl -d Ubuntu-22.04 -- bash -c "curl -Lo /mnt/c/Users/xander/Music/DICRON/kernel/src/limine.h \
    https://codeberg.org/Limine/limine-protocol/raw/branch/main/limine.h"
```

---

## Build System

### GNUmakefile

- Discovers `*.c` under `kernel/src/` and `dtest/` + `*.asm` under `kernel/src/`
- Freestanding x86-64 flags: `-ffreestanding -mcmodel=kernel -mno-red-zone -mno-sse -mno-mmx`
- Include paths: `-I kernel/include -I kernel/src` (public API via `<dicron/io.h>`, internals via relative paths)
- Links with `kernel/linker.lds` → `bin/dicron`
- `make iso` — builds bootable ISO via `xorriso` + Limine BIOS binaries
- `make run` — QEMU q35 BIOS boot from ISO, `-serial stdio`
- `make clean`

### linker.lds

Higher-half at `0xffffffff80000000`, standard Limine sections, entry `kmain`.

### limine.conf

BIOS boot, 5s timeout, path `boot():/boot/dicron`.

---

## Kernel Core

### main.c

Limine requests (framebuffer, memmap, HHDM). `kmain()` calls subsystem inits in order:

1. `gdt_init()`
2. `idt_init()`
3. `pmm_init()`
4. `console_init()`
5. `serial_init()` + `drv_register(DRV_SERIAL, ...)`
6. `kbd_init()` + `drv_register(DRV_KBD, ...)`
7. `kio_init()`
8. `dtest_main()`
9. Halt loop

### lib/

Freestanding `memcpy`/`memset`/`memmove`/`memcmp`/`strlen`/`strncmp`. Shared `vsnprintf` engine used by `kio_printf`, `klog`, and serial output internally.

---

## Subsystems

Console, memory, arch, and drivers function the same as v2 but are **internal only** — drivers register via `drv_register()`, and all external-facing output goes through the `kio`/`klog` API layer.

### Syscall Definitions

`syscall/syscall_nr.h` — reserved numbers (`__NR_exit` through `__NR_brk`). No implementation yet.

---

## Verification

### Automated

```bash
# Build
wsl -d Ubuntu-22.04 -- bash -c "cd /mnt/c/Users/xander/Music/DICRON && make clean && make"

# Boot test (serial, headless, 10s timeout)
wsl -d Ubuntu-22.04 -- bash -c "cd /mnt/c/Users/xander/Music/DICRON && \
    timeout 10 make run 2>&1 | head -50"
```

Expect: zero build errors, "DICRON" banner + memory stats on serial.

### Manual

- **Graphical QEMU**: boot shows Limine menu → DICRON banner on framebuffer
- **Keyboard**: keystrokes echo to screen and serial
