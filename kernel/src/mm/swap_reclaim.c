/*
 * swap_reclaim.c — Reclaimable page LRU list and eviction logic.
 *
 * Maintains a simple linked list of registered reclaimable pages.
 * swap_try_reclaim() drives the ZRAM → disk eviction chain:
 *   1. Pick the LRU (oldest) registered page
 *   2. Try ZRAM compression
 *   3. If ZRAM fails, try disk swap
 *   4. If both fail, give up (no panic)
 */

#include "swap.h"
#include "zram.h"
#include "pmm.h"
#include "vmm.h"
#include "lib/string.h"
#include <dicron/mem.h>
#include <dicron/errno.h>
#include <dicron/log.h>
#include <dicron/spinlock.h>
#include <generated/autoconf.h>

struct reclaim_entry {
	void *page;		/* HHDM virtual address of the reclaimable page */
	uint64_t virt;		/* user virtual address (0 = cooperative, no PTE update) */
	uint64_t pml4_phys;	/* physical PML4 of the owning address space */
	struct reclaim_entry *next;
};

#define RECLAIM_MAX_ENTRIES	128

static struct reclaim_entry entries[RECLAIM_MAX_ENTRIES];
static struct reclaim_entry *reclaim_head;	/* LRU: oldest at head */
static struct reclaim_entry *reclaim_free;	/* free list */
static spinlock_t reclaim_lock = SPINLOCK_INIT;
static int reclaim_initialized;

static void reclaim_init_once(void)
{
	if (reclaim_initialized)
		return;

	memset(entries, 0, sizeof(entries));
	reclaim_head = NULL;
	reclaim_free = &entries[0];

	for (int i = 0; i < RECLAIM_MAX_ENTRIES - 1; i++)
		entries[i].next = &entries[i + 1];
	entries[RECLAIM_MAX_ENTRIES - 1].next = NULL;

	reclaim_initialized = 1;
}

/* Internal helper — caller must hold reclaim_lock */
static int swap_register_locked(void *page, uint64_t virt, uint64_t pml4_phys)
{
	struct reclaim_entry *e = reclaim_free;
	if (!e)
		return -ENOMEM;
	reclaim_free = e->next;

	e->page      = page;
	e->virt      = virt;
	e->pml4_phys = pml4_phys;
	e->next      = NULL;

	if (!reclaim_head) {
		reclaim_head = e;
	} else {
		struct reclaim_entry *tail = reclaim_head;
		while (tail->next)
			tail = tail->next;
		tail->next = e;
	}
	return 0;
}

int swap_register(void *page)
{
	if (!page)
		return -EINVAL;

	uint64_t flags = spin_lock_irqsave(&reclaim_lock);
	reclaim_init_once();
	int ret = swap_register_locked(page, 0, 0);
	spin_unlock_irqrestore(&reclaim_lock, flags);
	return ret;
}

int swap_register_mapped(void *page, uint64_t virt, uint64_t pml4_phys)
{
	if (!page || !virt || !pml4_phys)
		return -EINVAL;

	uint64_t flags = spin_lock_irqsave(&reclaim_lock);
	reclaim_init_once();
	int ret = swap_register_locked(page, virt, pml4_phys);
	spin_unlock_irqrestore(&reclaim_lock, flags);
	return ret;
}

int swap_unregister(void *page)
{
	if (!page)
		return -EINVAL;

	uint64_t flags = spin_lock_irqsave(&reclaim_lock);

	struct reclaim_entry **pp = &reclaim_head;
	while (*pp) {
		if ((*pp)->page == page) {
			struct reclaim_entry *e = *pp;
			*pp = e->next;

			/* Return to free list */
			e->page = NULL;
			e->next = reclaim_free;
			reclaim_free = e;

			spin_unlock_irqrestore(&reclaim_lock, flags);
			return 0;
		}
		pp = &(*pp)->next;
	}

	spin_unlock_irqrestore(&reclaim_lock, flags);
	return -ENOENT;
}

void *swap_try_reclaim(unsigned int order)
{
	/* Currently only support single-page reclaim */
	if (order != 0)
		return NULL;

	uint64_t flags = spin_lock_irqsave(&reclaim_lock);

	reclaim_init_once();

	/* Pop LRU victim (head of list) */
	struct reclaim_entry *victim = reclaim_head;
	if (!victim) {
		spin_unlock_irqrestore(&reclaim_lock, flags);
		return NULL;
	}

	reclaim_head = victim->next;
	void     *page      = victim->page;
	uint64_t  virt      = victim->virt;
	uint64_t  pml4_phys = victim->pml4_phys;

	/* Return entry to free list */
	victim->page      = NULL;
	victim->virt      = 0;
	victim->pml4_phys = 0;
	victim->next      = reclaim_free;
	reclaim_free      = victim;

	spin_unlock_irqrestore(&reclaim_lock, flags);

	if (!page)
		return NULL;

	/*
	 * Try ZRAM first (CONFIG_ZRAM).
	 * If ZRAM succeeds, the page data is safely compressed in heap.
	 * For mapped pages (virt != 0), replace the present PTE with a
	 * swap-encoded not-present PTE so the fault handler can restore it.
	 */
#ifdef CONFIG_ZRAM
	int zslot = zram_compress_page(page);
	if (zslot >= 0) {
		pmm_free_page(page);
		if (virt && pml4_phys)
			vmm_write_pte_in(pml4_phys, virt,
					 swap_pte_encode(0, zslot));
		klog(KLOG_DEBUG, "swap: reclaimed page %p via ZRAM (slot %d)\n",
		     page, zslot);
		return page;
	}
#endif

	/*
	 * ZRAM failed or not configured — try disk swap.
	 */
	if (swap_is_enabled()) {
		int dslot = swap_page_out(page);
		if (dslot >= 0) {
			pmm_free_page(page);
			if (virt && pml4_phys)
				vmm_write_pte_in(pml4_phys, virt,
						 swap_pte_encode(1, dslot));
			klog(KLOG_DEBUG, "swap: reclaimed page %p via disk (slot %d)\n",
			     page, dslot);
			return page;
		}
	}

	/*
	 * Both paths failed — re-register the page (preserving virt/pml4_phys)
	 * and give up.
	 */
	{
		uint64_t lk = spin_lock_irqsave(&reclaim_lock);
		(void)swap_register_locked(page, virt, pml4_phys);
		spin_unlock_irqrestore(&reclaim_lock, lk);
	}
	klog(KLOG_WARN, "swap: reclaim failed for page %p\n", page);
	return NULL;
}

/*
 * swap_pf_handle — transparent swap-in from the page fault handler.
 *
 * Called with interrupts disabled (inside the CPU exception handler).
 * Reads the raw PTE at fault_addr.  If it carries a swap marker, allocates
 * a fresh frame, reads the page data back from ZRAM or disk, writes a new
 * present PTE, and returns 1 so the faulting instruction can be retried.
 * Returns 0 for any address that was not swap-evicted by this subsystem.
 */
int swap_pf_handle(uint64_t fault_addr, uint64_t pml4_phys)
{
	if (!pml4_phys)
		return 0;

	uint64_t page_addr = fault_addr & ~(uint64_t)0xFFF;
	uint64_t pte = vmm_read_pte_in(pml4_phys, page_addr);

	if (!swap_pte_is_swap(pte))
		return 0;

	int is_disk = (pte & SWAP_PTE_DISK) != 0;
	int slot    = swap_pte_slot(pte);

	/* Allocate a fresh physical frame to hold the restored page */
	void *new_page = pmm_alloc_page();
	if (!new_page) {
		klog(KLOG_ERR, "swap_pf: OOM restoring page at 0x%lx\n",
		     (unsigned long)fault_addr);
		return 0;
	}

	int ret;
	if (is_disk) {
		ret = swap_page_in(slot, new_page);
		if (ret == 0)
			swap_free_slot(slot);
	} else {
#ifdef CONFIG_ZRAM
		ret = zram_decompress_page(slot, new_page);
		if (ret == 0)
			zram_free_slot(slot);
#else
		ret = -ENODEV;
#endif
	}

	if (ret != 0) {
		pmm_free_page(new_page);
		klog(KLOG_ERR,
		     "swap_pf: swap-in failed at 0x%lx (slot %d, %s) err %d\n",
		     (unsigned long)fault_addr, slot,
		     is_disk ? "disk" : "ZRAM", ret);
		return 0;
	}

	/* Remap the page as user-writable and present */
	uint64_t phys  = (uint64_t)new_page - vmm_get_hhdm();
	uint64_t flags = VMM_PRESENT | VMM_WRITE | VMM_USER;
	vmm_write_pte_in(pml4_phys, page_addr, phys | flags);

	klog(KLOG_DEBUG, "swap_pf: restored 0x%lx from %s slot %d\n",
	     (unsigned long)fault_addr, is_disk ? "disk" : "ZRAM", slot);
	return 1;
}
