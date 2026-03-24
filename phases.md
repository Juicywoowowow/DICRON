# DICRON — 7 Phases to a Viable Kernel

Current state: boots via Limine, has GDT/IDT, bitmap PMM, single-page bump allocator, framebuffer console, serial, PS/2 keyboard, kio/klog API. No userspace, no filesystem, no scheduler.

---

## Phase 0.5 — Cleanup & Professional Foundation

**Goal**: Harden the codebase, fix inconsistencies, and establish conventions that every future phase builds on. No new features — just discipline.

### 0.5.1 — Kernel type system

- [ ] **`dicron/types.h`** — central typedefs: `ssize_t`, `pid_t`, `off_t`, `size_t`, `uint*_t` re-exports. Remove the ad-hoc `typedef long ssize_t` from `dev.h`
- [ ] **`dicron/errno.h`** — standard error codes: `-ENOMEM`, `-EINVAL`, `-ENODEV`, `-ENOSYS`, `-EBUSY`. All kernel functions return `int` (0 = success, negative = error) instead of magic `-1`
- [ ] **All API headers include `dicron/types.h`** — single source of truth for kernel types

### 0.5.2 — Include path cleanup

- [ ] **Eliminate deep relative includes** — `../../arch/x86_64/io.h` becomes `arch/x86_64/io.h` by adding `-I kernel/src` (already there, but code doesn't use it)
- [ ] **Internal headers use short paths** — `#include "mm/pmm.h"` not `#include "../mm/pmm.h"`
- [ ] **Rule: `<dicron/*.h>` = public API, `"subsys/file.h"` = internal** — no exceptions

### 0.5.3 — Code style normalization

- [ ] **Tabs everywhere** — some files got spaces during editing. Enforce tabs for indentation (Linux kernel style)
- [ ] **Consistent brace style** — K&R: opening brace on same line for functions and control flow
- [ ] **Header guard convention** — all guards follow `_DICRON_SUBSYS_FILE_H` pattern (e.g. `_DICRON_MM_PMM_H`, `_DICRON_ARCH_X86_64_GDT_H`)

### 0.5.4 — API signature cleanup

- [ ] **`pmm_init`** — change signature from `(uint64_t *, size_t, uint64_t)` to `(struct limine_memmap_response *, uint64_t hhdm)`. Don't force callers to cast and don't cast internally
- [ ] **`console_init`** — take a `struct limine_framebuffer *` directly instead of 4 separate args
- [ ] **`drv_register` / `drv_get`** — return `-EINVAL`/`-EBUSY` instead of `-1`
- [ ] **`kio_printf` / `klog`** — already good, no changes needed

### 0.5.5 — Driver model tightening

- [ ] **All `kdev_ops` callbacks return `int` or `ssize_t`** with errno-style errors — document this in `dev.h`
- [ ] **`kdev.id` → `kdev.minor`** — `id` is vague, `minor` matches Unix convention
- [ ] **Add `kdev.type`** — `KDEV_CHAR` / `KDEV_BLOCK` enum for Phase 5 VFS readiness

### 0.5.6 — Build system hardening

- [ ] **`-Wno-unused-parameter` removed** — fix all `(void)param` casts properly or use `__attribute__((unused))`
- [ ] **Add `-Wconversion -Wsign-conversion`** — catch type mismatch bugs early
- [ ] **Dependency tracking** — add `-MMD -MP` to CFLAGS, `-include $(OBJS:.o=.d)` to Makefile so header changes trigger rebuilds
- [ ] **`make fmt`** — optional: add a clang-format or indent target with a `.clang-format` config

### 0.5.7 — Kernel panic & assert

- [ ] **`dicron/panic.h`** — `void kpanic(const char *fmt, ...) __attribute__((noreturn))` — prints message to serial+console, dumps registers, halts. Used instead of silent `halt()` loops
- [ ] **`KASSERT(expr)`** macro — calls `kpanic` with file/line on failure. Use in PMM, kheap, driver registry for invariant checks
- [ ] **Replace all bare `halt()` with `kpanic("reason")`** in main.c error paths

### 0.5.8 — Project structure adjustments

Current:
```
kernel/src/lib/string.h    ← internal helpers mixed flat
kernel/src/lib/printf.h
```

After:
```
kernel/src/lib/string.h    ← no change (these are fine)
kernel/src/lib/printf.h
kernel/include/dicron/      
├── types.h                ← NEW: central types
├── errno.h                ← NEW: error codes
├── panic.h                ← NEW: kpanic + KASSERT
├── io.h                   ← existing (cleaned)
├── mem.h                  ← existing (cleaned)
├── log.h                  ← existing (cleaned)
└── dev.h                  ← existing (cleaned, uses types.h)
```

No directory restructuring — just new headers and convention enforcement.

### Checklist before starting Phase 1

- [x] `make clean && make` — zero warnings with `-Wall -Wextra -Werror -Wconversion`
- [x] Every public API function returns errno-style errors (or is documented why not)
- [x] No relative includes deeper than one level (`../` ok, `../../` not ok)
- [x] `kpanic()` exists and is used in all fatal error paths
- [x] `KASSERT()` guards PMM bitmap bounds, kheap null checks, driver registry bounds

**Deliverable**: Same functionality, zero new features, but the codebase is clean enough that Phase 1–7 code won't fight the foundations.

---

## Phase 1 — Proper Memory Management

**Goal**: Replace the bump allocator with real memory infrastructure.

- [x] **Slab allocator** — replace kheap with a slab/buddy allocator that handles arbitrary sizes, multi-page allocations, and tracks free blocks properly
- [x] **VMM implementation** — walk/create x86-64 4-level page tables, map/unmap pages on demand
- [x] **Kernel heap** — growable heap region in virtual address space (e.g. `0xffffc00000000000+`)
- [x] **Stack guard pages** — unmap pages below kernel stacks to catch overflows
- [x] **Memory stats** — track used/free/reserved memory, expose via klog

**Deliverable**: `kmalloc(any_size)` works reliably, `vmm_map_page`/`vmm_unmap_page` functional.

---

## Phase 2 — Interrupts, Timers & Preemption

**Goal**: Time-based preemption so the kernel doesn't rely on cooperative yields.

- [x] **PIT or HPET timer** — configure a hardware timer at ~1000 Hz (tick every 1ms)
- [x] **Kernel tick counter** — global `jiffies` / `ticks` variable
- [x] **Sleep/delay functions** — `ksleep_ms(n)`, `kdelay_us(n)`
- [x] **Spinlocks** — basic `spinlock_t` with `spin_lock()`/`spin_unlock()` using atomic ops
- [x] **Interrupt-safe locking** — `spin_lock_irqsave()` / `spin_unlock_irqrestore()`

**Deliverable**: Timer IRQ fires reliably, `ksleep_ms()` works, shared data structures are safe.

---

## Phase 3 — Process & Scheduler

**Goal**: Multiple kernel threads with round-robin scheduling.

- [ ] **Task struct** — `struct task { pid, state, kernel_stack, page_table, context, ... }`
- [ ] **Context switching** — save/restore registers (RSP, RIP, callee-saved), switch page tables
- [ ] **Kernel threads** — `kthread_create(fn, arg)` — spawn a new kernel-mode thread
- [ ] **Round-robin scheduler** — run queue, timer-driven preemption via PIT IRQ, `schedule()` function
- [ ] **Idle task** — runs `hlt` when nothing else to do
- [ ] **Task states** — RUNNING, READY, SLEEPING, DEAD
- [ ] **Wait queues** — `kwait()`/`kwake()` for blocking I/O (keyboard, etc.)

**Deliverable**: Multiple kernel threads run concurrently, keyboard input uses wait queues instead of busy-loop `hlt`.

---

## Phase 4 — Userspace

**Goal**: Ring 3 processes with syscall interface.

- [ ] **TSS setup** — configure Task State Segment for ring 0 ↔ ring 3 transitions
- [ ] **User page tables** — per-process address space, lower-half user mappings
- [ ] **`syscall`/`sysret` or `int 0x80`** — syscall entry/exit path, save/restore user context
- [ ] **ELF loader** — parse ELF64 headers, map `.text`/`.data`/`.bss`/`.rodata` segments
- [ ] **First syscalls** — `sys_exit`, `sys_write` (to serial/console), `sys_read` (from keyboard)
- [ ] **User stack** — map user stack pages at top of user address space
- [ ] **Static test binary** — embed a flat binary or simple ELF as a Limine module, exec it in ring 3

**Deliverable**: A user-mode "hello world" process runs, writes to console via `sys_write`, exits via `sys_exit`.

---

## Phase 5 — Virtual Filesystem & Initrd

**Goal**: Unified file abstraction, RAM-based initial filesystem.

- [ ] **VFS layer** — `struct vnode`, `struct vfs_ops` with `open`/`read`/`write`/`close`/`stat`/`readdir`
- [ ] **File descriptor table** — per-process fd table, `fd_alloc()`/`fd_get()`
- [ ] **Initrd / ramfs** — tar or cpio archive loaded as Limine module, mounted at `/`
- [ ] **Devfs** — `/dev/console`, `/dev/serial0`, `/dev/null` backed by kdev drivers
- [ ] **Path resolution** — `vfs_lookup("/dev/console")` walks the mount tree
- [ ] **Expand syscalls** — `sys_open`, `sys_close`, `sys_read`, `sys_write` now work on fds

**Deliverable**: Userspace can `open("/dev/console")` and read/write via file descriptors. Programs loaded from initrd.

---

## Phase 6 — IPC, Signals & Process Management

**Goal**: Processes can communicate, parent-child relationships work.

- [ ] **`sys_fork`** — clone process (copy page tables with COW or full copy)
- [ ] **`sys_exec`** — replace process image with new ELF from VFS
- [ ] **`sys_waitpid`** — parent waits for child exit
- [ ] **Pipes** — `sys_pipe` creates unidirectional byte stream between fds
- [ ] **Signals** — `SIGKILL`, `SIGTERM`, `SIGSEGV` with default handlers, `sys_kill`
- [ ] **`sys_brk` / `sys_mmap`** — user heap management
- [ ] **Init process** — PID 1 spawns from initrd, forks/execs child programs

**Deliverable**: Shell-like init process that can fork, exec programs from initrd, and pipe output between them.

---

## Phase 7 — Shell, Utilities & Stability

**Goal**: A usable (if minimal) operating system.

- [ ] **Minimal libc** — `printf`, `malloc`, `read`, `write`, `open`, `close`, `exit`, `fork`, `exec`, `waitpid` wrappers
- [ ] **Shell** — command-line interpreter: parse input, fork+exec commands, wait for completion
- [ ] **Core utilities** — `echo`, `cat`, `ls`, `clear`, `uname`, `ps` (list tasks)
- [ ] **Kernel panic handler** — catch double faults, print register dump + stack trace
- [ ] **Page fault handler** — print fault address, kill offending process instead of triple-faulting
- [ ] **Stress testing** — rapid fork/exit cycles, memory exhaustion recovery, interrupt storms

**Deliverable**: Boot → Limine → kernel → init → shell prompt. User types commands, sees output. Processes are isolated and crashes don't bring down the kernel.

---

## Summary

| Phase | What you get |
|-------|-------------|
| 1 | Real memory allocator + virtual memory |
| 2 | Timer, preemption, locks |
| 3 | Multitasking kernel threads |
| 4 | Ring 3 userspace + syscalls |
| 5 | Filesystem + file descriptors |
| 6 | fork/exec/pipe/signals |
| 7 | Shell + utilities = usable OS |

Each phase builds on the previous. Don't skip ahead — Phase 3 needs Phase 2's timer, Phase 4 needs Phase 3's scheduler, etc.
