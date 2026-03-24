#include <dicron/process.h>
#include <dicron/elf.h>
#include <dicron/sched.h>
#include <dicron/log.h>
#include <dicron/mem.h>
#include "mm/vmm.h"
#include "mm/pmm.h"
#include "arch/x86_64/tss.h"
#include "lib/string.h"

/*
 * process.c — Process creation and management.
 *
 * A process owns:
 *   - A PML4 (page table root) with kernel-half shared
 *   - A user stack (mapped at the top of user space)
 *   - A heap starting after the ELF segments (managed by sys_brk)
 *   - A main kernel thread that enters usermode
 */

#define USER_STACK_TOP   0x00007FFFFFFFE000ULL
#define USER_STACK_PAGES 4                        /* 16 KiB user stack */

/* Assembly trampoline to jump to ring 3 */
extern void enter_usermode(uint64_t rip, uint64_t rsp);

/* Kernel RSP for syscall entry */
extern uint64_t syscall_kernel_rsp;

static int next_pid = 1;

/*
 * The thread function that switches to the process's address space
 * and enters usermode. Runs as a kernel thread initially.
 */
static void process_entry(void *arg)
{
	struct process *proc = (struct process *)arg;

	/*
	 * Disable interrupts for the entire kernel→usermode transition.
	 * We must not be preempted between switching CR3, setting up
	 * syscall_kernel_rsp, and the iretq in enter_usermode.
	 * The iretq pops RFLAGS with IF=1, atomically re-enabling interrupts
	 * once we're in ring 3.
	 */
	__asm__ volatile("cli");

	/* Switch to the process's page tables */
	vmm_switch_to(proc->cr3);

	/* Set up the kernel RSP for syscall returns.
	 * kernel_stack is already the top of the stack (returned by kstack_alloc). */
	struct task *cur = sched_current();
	if (cur) {
		syscall_kernel_rsp = (uint64_t)cur->kernel_stack;
		/* TSS.RSP0 must also point to the kernel stack so that
		 * hardware interrupts (e.g. PIT timer) arriving in ring 3
		 * have a valid kernel stack to switch to. */
		tss_set_rsp0((uint64_t)cur->kernel_stack);
	}

	/* Jump to ring 3 — iretq re-enables interrupts atomically */
	enter_usermode(proc->entry, proc->stack_top);

	/* Should never return */
	klog(KLOG_ERR, "process_entry: enter_usermode returned!\n");
	kthread_exit();
}

struct process *process_create(const void *elf_data, size_t elf_size)
{
	struct process *proc = kzalloc(sizeof(struct process));
	if (!proc)
		return NULL;

	proc->pid = next_pid++;

	/* Create a new address space */
	proc->cr3 = vmm_create_user_pml4();
	if (proc->cr3 == 0) {
		klog(KLOG_ERR, "process_create: failed to create PML4\n");
		kfree(proc);
		return NULL;
	}

	/* Load ELF into the new address space */
	uint64_t brk = 0;
	proc->entry = elf64_load_into(elf_data, elf_size, proc->cr3, &brk);
	if (proc->entry == 0) {
		klog(KLOG_ERR, "process_create: ELF load failed\n");
		kfree(proc);
		return NULL;
	}
	proc->brk_base = brk;
	proc->brk = brk;

	/* Map user stack pages */
	uint64_t hhdm = vmm_get_hhdm();
	uint64_t initial_rsp = USER_STACK_TOP;
	for (int i = 0; i < USER_STACK_PAGES; i++) {
		void *page = pmm_alloc_page();
		if (!page) {
			klog(KLOG_ERR, "process_create: OOM allocating user stack\n");
			kfree(proc);
			return NULL;
		}
		memset(page, 0, 4096);
		uint64_t va = USER_STACK_TOP - (uint64_t)(i + 1) * 4096;
		
		if (i == 0) {
			/* Create standard Linux initial stack frame for musl */
			uint8_t *page_top = (uint8_t *)page + 4096;
			const char *prog = ""; /* Empty string for name */
			size_t prog_len = strlen(prog) + 1;
			page_top -= prog_len;
			memcpy(page_top, prog, prog_len);
			
			uint64_t prog_va = USER_STACK_TOP - prog_len;
			
			/* 16-byte align the stack pointer before pushing */
			uint64_t *stack = (uint64_t *)((uint64_t)page_top & ~15ULL);

			*(--stack) = 0; /* auxv[0].a_val */
			*(--stack) = 0; /* auxv[0].a_type = AT_NULL */
			*(--stack) = 0; /* envp[0] NULL */
			*(--stack) = 0; /* argv[1] NULL */
			*(--stack) = prog_va; /* argv[0] pointing to "" */
			*(--stack) = 1; /* argc = 1 */
			
			size_t offset_from_top = (size_t)((uint8_t *)page + 4096 - (uint8_t *)stack);
			initial_rsp = USER_STACK_TOP - offset_from_top;
		}

		uint64_t phys = (uint64_t)page - hhdm;
		vmm_map_page_in(proc->cr3, va, phys, VMM_PRESENT | VMM_WRITE | VMM_USER | VMM_NX);
	}
	proc->stack_top = initial_rsp;

	/* Create a kernel thread that will enter usermode */
	proc->main_thread = kthread_create(process_entry, proc);
	if (!proc->main_thread) {
		klog(KLOG_ERR, "process_create: failed to create main thread\n");
		kfree(proc);
		return NULL;
	}
	proc->main_thread->process = proc;

	process_fd_init(proc);

	klog(KLOG_INFO, "Process %d created (entry=0x%lx, stack=0x%lx)\n",
	     proc->pid, proc->entry, proc->stack_top);

	return proc;
}

void process_destroy(struct process *proc)
{
	if (!proc)
		return;
	process_fd_cleanup(proc);
	/* TODO: tear down user-half page tables, free all user pages */
	kfree(proc);
}

struct process *process_current(void)
{
	struct task *cur = sched_current();
	if (cur)
		return cur->process;
	return NULL;
}
