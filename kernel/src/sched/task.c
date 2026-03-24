#include "lib/string.h"
#include "mm/pmm.h"
#include "mm/slab.h"
#include "mm/vmm.h"
#include <dicron/io.h>
#include <dicron/log.h>
#include <dicron/mem.h>
#include <dicron/panic.h>
#include <dicron/sched.h>

static pid_t next_pid = 0;
static uint64_t valloc_next_vaddr =
    0xFFFFC00000000000ULL; /* custom virtual range */

/*
 * Virtual address free-list for recycled stack slots.
 * Each freed stack base address gets pushed here so the next kstack_alloc
 * can reuse the virtual range (and its existing page tables) instead of
 * bumping valloc_next_vaddr and forcing VMM to allocate new PT pages.
 */
#define VSTACK_FREELIST_CAP 512
static uint64_t vstack_freelist[VSTACK_FREELIST_CAP];
static int vstack_freelist_count = 0;
static size_t vstack_slot_pages = 0; /* pages-per-slot (set on first alloc) */

/* Allocate a stack with a guard page at the bottom */
void *kstack_alloc(size_t pages) {
  if (pages > 32) {
    klog(KLOG_ERR, "kstack_alloc: requested %lu pages exceeds 32-page limit\n",
         pages);
    return NULL;
  }

  /* Lock in the slot size on first call */
  if (vstack_slot_pages == 0 && pages >= 1)
    vstack_slot_pages = pages;

  uint64_t stack_base;

  /* Try to reuse a previously freed virtual range first.
   * Only recycle when pages > 0 and matches the standard slot size,
   * so that a zero-page edge-case allocation never pollutes the freelist. */
  if (pages > 0 && vstack_freelist_count > 0 && pages == vstack_slot_pages) {
    vstack_freelist_count--;
    stack_base = vstack_freelist[vstack_freelist_count];
  } else {
    stack_base = valloc_next_vaddr;
    valloc_next_vaddr += (pages + 1) * 4096; /* +1 for guard page */
  }

  /* Array to track mapped phys pages in case of OOM midway */
  void *phys_pages[32];

  for (size_t i = 1; i <= pages; i++) {
    phys_pages[i - 1] = pmm_alloc_page();
    if (!phys_pages[i - 1]) {
      /* OOM: rollback previously allocated pages for this stack */
      for (size_t j = 0; j < i - 1; j++) {
        pmm_free_page(phys_pages[j]);
        vmm_unmap_page(stack_base + (j + 1) * 4096);
      }
      return NULL;
    }

    uint64_t phys_paddr = (uint64_t)phys_pages[i - 1] - pmm_hhdm_offset();
    vmm_map_page(stack_base + i * 4096, phys_paddr,
                 VMM_PRESENT | VMM_WRITE | VMM_NX);
  }

  return (void *)(stack_base + (pages + 1) * 4096);
}

void kstack_free(void *stack, size_t pages) {
  if (!stack)
    return;

  /* Top of stack was returned, so base is: */
  uint64_t top = (uint64_t)stack;
  uint64_t stack_base = top - ((pages + 1) * 4096);

  for (size_t i = 1; i <= pages; i++) {
    uint64_t vaddr = stack_base + i * 4096;
    uint64_t paddr = vmm_virt_to_phys(vaddr);
    if (paddr) {
      void *hhdm_ptr = (void *)(paddr + pmm_hhdm_offset());
      pmm_free_page(hhdm_ptr);
      vmm_unmap_page(vaddr);
    }
  }

  /* Push stack_base back onto the free-list for reuse if it matches standard
   * slot size.  Reject zero-page slots to avoid virtual address overlap. */
  if (pages > 0 && pages == vstack_slot_pages &&
      vstack_freelist_count < VSTACK_FREELIST_CAP) {
    vstack_freelist[vstack_freelist_count] = stack_base;
    vstack_freelist_count++;
  }
}

extern void kernel_thread_stub(void);

struct task *kthread_create(void (*fn)(void *), void *arg) {
  if (!fn) {
    klog(KLOG_ERR, "kthread_create: fn cannot be NULL\n");
    return NULL;
  }

  struct task *t = kmalloc(sizeof(struct task));
  if (!t)
    return NULL;

  memset(t, 0, sizeof(struct task));
  t->pid = next_pid++;
  t->state = TASK_READY;
  t->priority = 0; /* new tasks enter at highest priority */
  t->remaining = mlfq_timeslice[0];

  /* Allocate 4 pages (16 KiB) for kernel stack utilizing the vmm/pmm mapping
   * engine */
  t->kernel_stack = kstack_alloc(4);
  if (!t->kernel_stack) {
    kfree(t);
    return NULL;
  }

  /* Set up initial CPU context */
  uint64_t *sp =
      (uint64_t *)((uint8_t *)t->kernel_stack - sizeof(struct cpu_context));
  t->context = (struct cpu_context *)sp;
  memset(t->context, 0, sizeof(struct cpu_context));

  /* Execution starts at the stub */
  t->context->rip = (uint64_t)kernel_thread_stub;

  /* Arguments to the stub are passed via R12 and R13 */
  t->context->r12 = (uint64_t)fn;
  t->context->r13 = (uint64_t)arg;

  return t;
}

struct task *kthread_spawn(void (*fn)(void *), void *arg) {
  struct task *t = kthread_create(fn, arg);
  if (!t)
    return NULL;
  mlfq_enqueue(t);
  return t;
}

void kthread_yield(void) { schedule(); }

void kthread_exit(void) {
  struct task *cur = sched_current();
  if (cur) {
    cur->state = TASK_DEAD;
    mlfq_remove(cur);
  }
  schedule();
  /* schedule() should never return here for a DEAD task */
  kpanic("kthread_exit: schedule returned to dead task");
}

void kthread_free(struct task *t) {
  if (!t)
    return;
  if (t->kernel_stack) {
    kstack_free(t->kernel_stack, 4);
  }
  kfree(t);
}
