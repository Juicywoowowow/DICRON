#include <dicron/syscall.h>
#include <dicron/process.h>
#include <dicron/sched.h>
#include <dicron/log.h>
#include <dicron/uaccess.h>
#include "mm/vmm.h"
#include "mm/pmm.h"
#include "lib/string.h"

/*
 * sys_memory.c — Memory management syscalls: brk, mmap, munmap.
 *
 * brk:    Adjusts the process heap.  Maps/unmaps pages as needed.
 * mmap:   Only MAP_ANONYMOUS|MAP_PRIVATE supported (no file backing).
 * munmap: Unmaps pages and frees the backing physical frames.
 */

#define PAGE_SIZE  4096
#define PAGE_MASK  (~(uint64_t)(PAGE_SIZE - 1))

/* Linux mmap flags */
#define MAP_ANONYMOUS  0x20
#define MAP_PRIVATE    0x02
#define MAP_FIXED      0x10

#define PROT_READ   0x1
#define PROT_WRITE  0x2
#define PROT_EXEC   0x4

/* Simple bump allocator for anonymous mmap regions */
static uint64_t mmap_next = 0x0000200000000000ULL;

/* ------------------------------------------------------------------ */
/*  sys_brk                                                            */
/* ------------------------------------------------------------------ */

long sys_brk(long addr, long a1, long a2, long a3, long a4, long a5)
{
	(void)a1; (void)a2; (void)a3; (void)a4; (void)a5;

	struct process *proc = process_current();
	if (!proc)
		return -ENOMEM;

	/* brk(0) — query current break */
	if (addr == 0)
		return (long)proc->brk;

	uint64_t new_brk = (uint64_t)addr;

	/* Sanity: don't allow shrinking below brk_base or into kernel space */
	if (new_brk < proc->brk_base)
		return (long)proc->brk;
	if (new_brk >= USER_SPACE_END)
		return (long)proc->brk;

	uint64_t old_page_end = (proc->brk + PAGE_SIZE - 1) & PAGE_MASK;
	uint64_t new_page_end = (new_brk + PAGE_SIZE - 1) & PAGE_MASK;

	uint64_t hhdm = vmm_get_hhdm();

	if (new_page_end > old_page_end) {
		/* Expanding: map new zeroed pages */
		for (uint64_t va = old_page_end; va < new_page_end; va += PAGE_SIZE) {
			void *page = pmm_alloc_page();
			if (!page)
				return (long)proc->brk; /* OOM: keep old brk */
			memset(page, 0, PAGE_SIZE);
			uint64_t phys = (uint64_t)page - hhdm;
			vmm_map_page_in(proc->cr3, va, phys,
					VMM_PRESENT | VMM_WRITE | VMM_USER | VMM_NX);
		}
	} else if (new_page_end < old_page_end) {
		/* Shrinking: free physical pages and unmap PTEs */
		for (uint64_t va = new_page_end; va < old_page_end; va += PAGE_SIZE) {
			uint64_t phys = vmm_virt_to_phys_in(proc->cr3, va);
			if (phys) {
				void *hhdm_ptr = (void *)((phys & PAGE_MASK) + hhdm);
				pmm_free_page(hhdm_ptr);
			}
			vmm_unmap_page_in(proc->cr3, va);
		}
	}

	proc->brk = new_brk;
	return (long)new_brk;
}

/* ------------------------------------------------------------------ */
/*  sys_mmap                                                           */
/* ------------------------------------------------------------------ */

long sys_mmap(long addr, long length, long prot,
	      long flags, long fd, long offset)
{
	(void)addr; (void)fd; (void)offset;

	struct process *proc = process_current();
	if (!proc)
		return -ENOMEM;

	/* Only support anonymous private mappings */
	if (!(flags & MAP_ANONYMOUS))
		return -ENOMEM;

	if (length <= 0)
		return -EINVAL;

	size_t len = (size_t)length;
	size_t num_pages = (len + PAGE_SIZE - 1) / PAGE_SIZE;

	/* Pick a virtual address */
	uint64_t va_start = mmap_next;
	mmap_next += num_pages * PAGE_SIZE;

	if (va_start + num_pages * PAGE_SIZE >= USER_SPACE_END)
		return -ENOMEM;

	uint64_t hhdm = vmm_get_hhdm();
	uint64_t vmflags = VMM_PRESENT | VMM_USER;
	if (prot & PROT_WRITE)
		vmflags |= VMM_WRITE;
	if (!(prot & PROT_EXEC))
		vmflags |= VMM_NX;

	for (size_t i = 0; i < num_pages; i++) {
		void *page = pmm_alloc_page();
		if (!page)
			return -ENOMEM;
		memset(page, 0, PAGE_SIZE);
		uint64_t phys = (uint64_t)page - hhdm;
		vmm_map_page_in(proc->cr3, va_start + i * PAGE_SIZE, phys, vmflags);
	}

	return (long)va_start;
}

/* ------------------------------------------------------------------ */
/*  sys_munmap                                                         */
/* ------------------------------------------------------------------ */

long sys_munmap(long addr, long length, long a2,
		long a3, long a4, long a5)
{
	(void)a2; (void)a3; (void)a4; (void)a5;

	struct process *proc = process_current();
	if (!proc)
		return -ENOMEM;

	if (length <= 0)
		return -EINVAL;
	if (addr & (PAGE_SIZE - 1))
		return -EINVAL;

	uint64_t va_start = (uint64_t)addr;
	uint64_t va_end = va_start + (uint64_t)length;
	va_end = (va_end + PAGE_SIZE - 1) & PAGE_MASK;

	uint64_t hhdm = vmm_get_hhdm();

	for (uint64_t va = va_start; va < va_end; va += PAGE_SIZE) {
		uint64_t phys = vmm_virt_to_phys_in(proc->cr3, va);
		if (phys) {
			void *hhdm_ptr = (void *)((phys & PAGE_MASK) + hhdm);
			pmm_free_page(hhdm_ptr);
		}
		vmm_unmap_page_in(proc->cr3, va);
	}

	return 0;
}
