Process Management
===================

1. Overview
-----------

The process management subsystem handles creation and lifecycle of
user processes. Each process has its own address space, file descriptors,
and main thread.

2. Process Structure
---------------------

The process structure contains:
  - pid: Process ID
  - fds: Array of file descriptors (max 64)
  - cr3: Physical address of page table (PML4)
  - brk: Current program break (heap end)
  - brk_base: Initial program break after loading
  - entry: ELF entry point
  - stack_top: Top of user stack
  - main_thread: Associated task structure
  - exit_code: Exit status code

3. Process Creation
-------------------

Processes are created from ELF binaries:
  struct process *process_create(const void *elf_data, size_t elf_size)

The process creation involves:
  1. Validate ELF header (see ELF loader documentation)
  2. Create new address space (PML4)
  3. Map ELF program segments into address space
  4. Set up user stack
  5. Allocate kernel stack for main thread
  6. Initialize file descriptor table
  7. Return process structure

4. Process Destruction
-----------------------

When a process exits:
  void process_destroy(struct process *proc)

This frees:
  - All file descriptors
  - User page tables and pages
  - Kernel stack
  - Process structure itself

5. Current Process
-----------------

To get the current process:
  struct process *process_current(void)

This returns the process associated with the currently running task.

6. ELF Loading
--------------

The ELF loader (src/proc/elf.c) handles:
  - Validating ELF64 headers
  - Parsing program headers (PT_LOAD segments)
  - Mapping segments to virtual addresses
  - Setting appropriate page permissions
  - Determining program entry point

Validation checks:
  - Magic number (ELF magic)
  - Class (64-bit only)
  - Endianness (little-endian)
  - Type (ET_EXEC)
  - Machine (EM_X86_64)
  - Version
  - Program headers present
  - No overlapping segments

7. Userspace Execution
----------------------

When a process runs:
  1. Scheduler switches to the process's task
  2. Page table is switched (CR3)
  3. CPU runs in user mode (ring 3)
  4. Execution starts at ELF entry point

System calls transition between user and kernel mode:
  - Enter kernel via syscall instruction
  - TSS provides kernel stack (RSP0)
  - Return to user via sysret instruction
