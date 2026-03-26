x86_64 Architecture Support
============================

1. Overview
-----------

The arch/x86_64 directory contains CPU-specific code for the x86_64
(64-bit) architecture. This includes the Global Descriptor Table (GDT),
Interrupt Descriptor Table (IDT), Task State Segment (TSS), and system
call entry points.

2. Files
--------

  gdt.c / gdt.h       - Global Descriptor Table setup
  idt.c / idt.h       - Interrupt Descriptor Table and IRQ handlers
  tss.c / tss.h       - Task State Segment for ring transitions
  switch.asm          - Context switching routine
  syscall_entry.asm  - System call entry from userspace
  syscall_setup.c    - System call initialization
  usermode.asm        - Helper to return to userspace
  io.h                - I/O port access macros

3. Global Descriptor Table (GDT)
-------------------------------

The GDT defines segment descriptors for the CPU. DICRON sets up:

  - Kernel code segment (selector 0x08)
  - Kernel data segment (selector 0x10)
  - User data segment (selector 0x1B)
  - User code segment (selector 0x23)
  - TSS segment (selector 0x28)

The GDT is initialized by gdt_init() in gdt.c.

4. Task State Segment (TSS)
---------------------------

The TSS stores the kernel stack pointer (RSP0) for when the CPU transitions
from ring 3 (userspace) to ring 0 (kernel). The tss_set_rsp0() function
updates RSP0 for the current process.

5. Interrupt Descriptor Table (IDT)
-----------------------------------

The IDT defines interrupt handlers. DICRON uses the IDT for:

  - Hardware IRQ handlers (PIT, keyboard, etc.)
  - Software interrupt handlers (syscall)
  - CPU exceptions (page fault, divide error, etc.)

Each handler is registered via idt_set_handler(vector, handler).

The handler receives an idt_frame pointer containing all register values:

  struct idt_frame {
      uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
      uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
      uint64_t int_no, err_code;
      uint64_t rip, cs, rflags, rsp, ss;
  };

6. Context Switching
--------------------

The switch.asm file contains the switch_context function that switches
between kernel threads. It saves callee-saved registers to the old
context and restores them from the new context.

The cpu_context structure (defined in sched.h) must match the push/pop
order in switch_context:

  struct cpu_context {
      uint64_t r15, r14, r13, r12, rbx, rbp, rip;
  } __attribute__((packed));

7. System Call Entry
--------------------

System calls use the syscall instruction. The entry point is set up in
syscall_setup.c, which configures the IDT to route syscall interrupts
to the kernel syscall handler.

The usermode.asm file provides helper routines to return to userspace
after handling a system call.

8. I/O Ports
------------

The io.h header provides macros for x86 I/O port access:

  - inb(port):   Read a byte from an I/O port
  - outb(port, val): Write a byte to an I/O port
  - inw(port):   Read a word from an I/O port
  - outw(port, val): Write a word to an I/O port
  - inl(port):   Read a dword from an I/O port
  - outl(port, val): Write a dword to an I/O port

These are used by device drivers for communication with hardware.
