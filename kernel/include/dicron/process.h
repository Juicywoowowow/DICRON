#ifndef _DICRON_PROCESS_H
#define _DICRON_PROCESS_H

#include <stdint.h>
#include <stddef.h>

#include <dicron/fs.h>

struct task;

struct process {
	int pid;
	struct file *fds[MAX_FDS];
	uint64_t cr3;             /* page table root (physical) */
	uint64_t brk;             /* current program break */
	uint64_t brk_base;        /* initial program break (end of loaded segments) */
	uint64_t entry;           /* ELF entry point */
	uint64_t stack_top;       /* top of user stack */
	struct task *main_thread;
	int exit_code;
};

/*
 * Create a new process from an ELF binary blob.
 * Allocates a new address space, loads ELF segments, sets up user stack.
 * Returns the process, or NULL on failure.
 */
struct process *process_create(const void *elf_data, size_t elf_size);

/* Destroy a process — tear down address space, free pages */
void process_destroy(struct process *proc);

/* Get the current process */
struct process *process_current(void);

#endif
