# DICRON Userspace Implementation Plan

Phased approach to get from kernel-only to running real Linux ELF binaries.

---

## Phase U1 — CPU Plumbing (Ring 3 Foundation)

**Goal:** The CPU can transition between ring 0 and ring 3.

### Step 1: GDT User Segments
- Add ring 3 code segment (selector 0x1B = index 3, RPL 3)
- Add ring 3 data segment (selector 0x23 = index 4, RPL 3)
- Both are flat 64-bit segments, identical to kernel ones but with DPL=3

### Step 2: TSS (Task State Segment)
- Create `struct tss` with RSP0 field (kernel stack pointer)
- Allocate a TSS, load it via `ltr`
- Add TSS descriptor to GDT (it's a 16-byte entry in long mode)
- On every context switch, update `tss.rsp0` to the current thread's kernel stack top

### Step 3: SYSCALL/SYSRET MSR Setup
- Write `STAR` MSR (0xC0000081): kernel CS/SS in bits 47:32, user CS/SS in bits 63:48
- Write `LSTAR` MSR (0xC0000082): address of `syscall_entry` asm stub
- Write `SFMASK` MSR (0xC0000084): mask IF flag (disable interrupts on entry)
- Enable `SCE` bit in `EFER` MSR

### Step 4: syscall_entry.asm
- Save user RSP (from `swapgs` + per-CPU area, or just store in a known location)
- Switch to kernel stack (from TSS RSP0)
- Push all caller-saved registers
- Call C `syscall_dispatch(rax, rdi, rsi, rdx, r10, r8, r9)`
- Restore registers, `sysretq` back to ring 3

### Step 5: IRETQ-to-Ring-3 Trampoline
- `enter_usermode(rip, rsp)`: pushes fake interrupt frame (SS, RSP, RFLAGS, CS, RIP) and does `iretq`
- This is how we initially jump into a userspace process

### Files Created
```
kernel/src/arch/x86_64/tss.c
kernel/src/arch/x86_64/tss.h
kernel/src/arch/x86_64/syscall_entry.asm
kernel/src/arch/x86_64/usermode.asm
```

### Files Modified
```
kernel/src/arch/x86_64/gdt.c  — user segments + TSS descriptor
kernel/src/arch/x86_64/gdt.h  — selector constants
```

### Tests
- `test_post_ring3.c`: iretq to a tiny ring 3 code snippet, syscall back, verify round-trip
- Verify TSS RSP0 is correctly set on context switch

### Definition of Done
- Can execute a function in ring 3 and return to ring 0 via `syscall`

---

## Phase U2 — Syscall Dispatch + Minimal Stubs

**Goal:** The kernel can receive and dispatch syscalls by number.

### Step 1: Syscall Number Header
- `kernel/include/dicron/syscall.h` with Linux x86_64-identical `__NR_xxx` defines
- Start with the ~30 most critical syscalls (see table below)

### Step 2: Syscall Dispatch Table
- `kernel/src/syscall/syscall.c`: array of function pointers indexed by syscall number
- `syscall_dispatch()`: bounds-check number, call handler, return result in RAX
- Unknown syscalls return `-ENOSYS`

### Step 3: Minimal Stubs
- Every syscall starts as a stub returning `-ENOSYS`
- Implement only these for Phase U2:
  - `sys_write` (nr 1) — write to serial/console (fd 1 = stdout, fd 2 = stderr)
  - `sys_exit_group` (nr 231) — mark process dead, schedule away

### Syscall Number Table (Linux x86_64)

| # | Name | Category | Phase |
|---|------|----------|-------|
| 0 | read | I/O | U4 |
| 1 | write | I/O | U2 |
| 2 | open | FS | U5 |
| 3 | close | I/O | U4 |
| 5 | fstat | FS | U5 |
| 8 | lseek | FS | U5 |
| 9 | mmap | Memory | U3 |
| 10 | mprotect | Memory | U5 |
| 11 | munmap | Memory | U3 |
| 12 | brk | Memory | U3 |
| 13 | rt_sigaction | Signal | U6+ |
| 14 | rt_sigprocmask | Signal | U6+ |
| 20 | writev | I/O | U4 |
| 21 | access | FS | U5 |
| 24 | sched_yield | Sched | U3 |
| 33 | dup2 | FS | U5 |
| 39 | getpid | Process | U3 |
| 56 | clone | Process | U6+ |
| 57 | fork | Process | U6+ |
| 59 | execve | Process | U4 |
| 60 | exit | Process | U2 |
| 63 | uname | Info | U3 |
| 79 | getcwd | FS | U5 |
| 96 | gettimeofday | Time | U4 |
| 158 | arch_prctl | Arch | U3 |
| 218 | set_tid_address | Thread | U3 |
| 231 | exit_group | Process | U2 |
| 257 | openat | FS | U5 |
| 302 | prlimit64 | Resource | U4 |

### Files Created
```
kernel/include/dicron/syscall.h
kernel/src/syscall/syscall.c
kernel/src/syscall/sys_io.c
kernel/src/syscall/sys_process.c
```

### Tests
- `test_post_syscall.c`: dispatch known/unknown numbers, verify -ENOSYS for stubs

### Definition of Done
- A ring 3 program can `syscall` with RAX=1 to write to console, RAX=231 to exit

---

## Phase U3 — Process + Memory + ELF Loading

**Goal:** Load and run a statically-linked ELF binary in its own address space.

### Step 1: Per-Process Address Space
- `vmm_create_address_space()`: allocate a new PML4, map the kernel half (higher-half shared)
- `vmm_switch_address_space(cr3)`: write CR3 to switch page tables
- `vmm_destroy_address_space()`: tear down user-half mappings, free pages

### Step 2: Process Struct
```c
struct process {
    pid_t pid;
    uint64_t cr3;           /* page table root */
    uint64_t brk;           /* program break for sys_brk */
    uint64_t entry;         /* ELF entry point */
    struct task *main_thread;
    struct process *parent;
};
```

### Step 3: ELF64 Loader
- Parse ELF header, validate magic + class (64-bit) + machine (x86_64)
- Walk program headers (PT_LOAD segments)
- For each segment: allocate user pages, map at segment vaddr, copy data
- Return entry point address

### Step 4: Implement Key Syscalls
- `sys_brk` (nr 12): grow/shrink the heap
- `sys_mmap` (nr 9): map anonymous pages (MAP_ANONYMOUS only initially)
- `sys_munmap` (nr 11): unmap pages
- `sys_getpid` (nr 39): return current PID
- `sys_uname` (nr 63): return "DICRON" system info
- `sys_arch_prctl` (nr 158): set FS base (needed for TLS/musl init)
- `sys_set_tid_address` (nr 218): stub, return current TID
- `sys_sched_yield` (nr 24): call kthread_yield

### Step 5: First User Process
- Embed a tiny test ELF in the kernel (or load from initrd)
- `process_exec()`: create address space → load ELF → create thread → iretq to entry

### Files Created
```
kernel/include/dicron/process.h
kernel/include/dicron/elf.h
kernel/include/dicron/uaccess.h
kernel/src/proc/process.c
kernel/src/proc/elf.c
kernel/src/proc/uaccess.c
kernel/src/syscall/sys_memory.c
user/hello.c
user/linker.lds
user/Makefile
```

### Files Modified
```
kernel/src/mm/vmm.c       — clone/destroy address spaces
kernel/src/sched/task.c    — process-aware thread creation
kernel/include/dicron/sched.h — link task↔process
kernel/src/main.c          — launch init process
GNUmakefile                — build user programs
```

### Tests
- `test_post_elf.c`: parse a known-good ELF header, validate segments
- `test_post_process.c`: create/destroy address spaces, verify isolation

### Definition of Done
- `user/hello.c` (hand-written syscall asm, no libc) runs and prints "Hello from DICRON userspace!"

---

## Phase U4 — musl libc + Real Programs

**Goal:** Cross-compile musl, run `printf("Hello World\n")` via musl.

### Step 1: VFS and File Descriptor Table (Completed!)
- We built the VFS (`struct file`, `struct inode`, `struct dentry`).
- Implemented process `fds` array and `file.c` manager.
- Implemented `/dev/console` and `ramfs`.
- Wired up `sys_read`, `sys_write`, `sys_open`, and `sys_close`.

### Step 2: Implement `sys_writev` (Completed!)
- `printf` relies on `sys_writev` (scatter/gather I/O) to output strings efficiently.
- Validates arrays, manages FDs safely, loops over IO vectors.

### Step 3: Implement Remaining musl Boot Syscalls (Completed!)
- `sys_gettimeofday` (nr 96), `sys_prlimit64` (nr 302), `sys_ioctl` (nr 16), `sys_readv` (nr 19).
- Tested dynamically at boot using system self-tests.

### Step 4: Cross-compile musl (Completed!)
- Configured musl with `x86_64-linux-musl` targeting standard Linux syscalls.
- Built as static library: `libc.a`.
- Utilized the `musl-gcc` wrapper.

### Step 5: Build and Run `hello_musl.c` (Completed!)
- Clobbered registers fixed: Managed `volatile` C calling boundaries in `syscall_entry.asm` to correctly preserve system V ABI constraints for user mode.
- Aligned user Mode execution `RSP` exactly on 16-byte boundaries. 
- Injected `argc`, `argv`, `envp`, `auxv` perfectly via pseudo boot blocks.
- Solved silent SSE Invalid Opcode exceptions `#UD` by proactively enabling SSE support via `CR0.EM`, `CR0.MP`, `CR4.OSFXSR`, and `CR4.OSXMMEXCPT`!
- Output flushes correctly, memory allocates (`malloc`), process tears down appropriately via `exit()`.

### Definition of Done (Completed!)
- A real C program compiled with musl and printf runs flawlessly on DICRON!

---

## Phase U5 — Filesystem Expansion (ramfs / initrd)

**Goal:** Programs can open/read/write directories and full trees from an in-memory filesystem.

- Expand the basic `ramfs` to support hierarchical directories and complex layouts.
- Load an initrd (tar or cpio archive) from Limine.
- Parse the archive and populate the `ramfs` tree on boot.
- Implement missing filesystem syscalls: `fstat`, `stat`, `getcwd`, `dup2`, `access`, `mprotect`, `lseek`.

---

## Phase U6+ — Advanced (Future)

- `fork` / `clone` — process duplication, copy-on-write pages
- Signals — `rt_sigaction`, `rt_sigprocmask`, signal delivery
- `pipe` — inter-process communication
- `wait4` / `waitpid` — process status tracking
- Shell — port `dash` or `busybox ash`
- Dynamic linking — `ld-musl-x86_64.so.1`

---

## Implementation Priority

```
U1 (CPU plumbing)  ──▶  U2 (syscall dispatch)  ──▶  U3 (ELF + process)
                                                          │
                                                          ▼
                                                    U4 (musl + real programs)
                                                          │
                                                          ▼
                                                    U5 (filesystem)
                                                          │
                                                          ▼
                                                    U6+ (fork, signals, shell)
```

Each phase is independently testable. We don't move to U(N+1) until U(N) passes all tests.
