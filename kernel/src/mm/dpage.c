/*
 * dpage.c — Demand paging descriptor table and fault handler.
 *
 * Maintains a static array of dpage_desc entries.  Each entry records
 * the source bytes and layout for one not-yet-backed virtual page.
 * On fault, dpage_pf_handle() populates the page and installs a real PTE.
 */

#include "dpage.h"
#include "vmm.h"
#include "pmm.h"
#include "lib/string.h"
#include <dicron/errno.h>
#include <dicron/log.h>
#include <dicron/spinlock.h>
#include <generated/autoconf.h>

/* ------------------------------------------------------------------ */
/*  Descriptor table                                                   */
/* ------------------------------------------------------------------ */

struct dpage_entry {
	struct dpage_desc desc;
	int               used;
};

static struct dpage_entry dtable[DPAGE_MAX_SLOTS];
static spinlock_t         dtable_lock = SPINLOCK_INIT;
static int                dtable_initialized;

static void dtable_init_once(void)
{
	if (dtable_initialized)
		return;
	memset(dtable, 0, sizeof(dtable));
	dtable_initialized = 1;
}

int dpage_alloc(const struct dpage_desc *desc)
{
	if (!desc)
		return -1;

	uint64_t flags = spin_lock_irqsave(&dtable_lock);
	dtable_init_once();

	for (int i = 0; i < DPAGE_MAX_SLOTS; i++) {
		if (!dtable[i].used) {
			dtable[i].desc = *desc;
			dtable[i].used = 1;
			spin_unlock_irqrestore(&dtable_lock, flags);
			return i;
		}
	}

	spin_unlock_irqrestore(&dtable_lock, flags);
	klog(KLOG_WARN, "dpage: descriptor table full\n");
	return -1;
}

int dpage_lookup(uint64_t slot, struct dpage_desc *desc_out)
{
	if (slot >= DPAGE_MAX_SLOTS || !desc_out)
		return -1;

	uint64_t flags = spin_lock_irqsave(&dtable_lock);
	if (!dtable[slot].used) {
		spin_unlock_irqrestore(&dtable_lock, flags);
		return -1;
	}
	*desc_out = dtable[slot].desc;
	spin_unlock_irqrestore(&dtable_lock, flags);
	return 0;
}

void dpage_free(uint64_t slot)
{
	if (slot >= DPAGE_MAX_SLOTS)
		return;

	uint64_t flags = spin_lock_irqsave(&dtable_lock);
	dtable[slot].used = 0;
	spin_unlock_irqrestore(&dtable_lock, flags);
}

/* ------------------------------------------------------------------ */
/*  Page fault handler                                                 */
/* ------------------------------------------------------------------ */

int dpage_pf_handle(uint64_t fault_addr, uint64_t pml4_phys,
		    uint64_t page_vmflags)
{
	if (!pml4_phys)
		return 0;

	uint64_t page_addr = fault_addr & ~(uint64_t)0xFFF;
	uint64_t pte       = vmm_read_pte_in(pml4_phys, page_addr);

	if (!dpage_pte_is_demand(pte))
		return 0;

	/* Allocate a fresh physical frame */
	void *frame = pmm_alloc_page();
	if (!frame) {
		klog(KLOG_ERR, "dpage_pf: OOM at 0x%lx\n",
		     (unsigned long)fault_addr);
		return 0;
	}

	/* Zero the whole page first — covers BSS tails and zero-pages */
	memset(frame, 0, 4096);

	if (!dpage_pte_is_zero(pte)) {
		/* ELF-backed: copy the file bytes into the right offset */
		uint64_t slot = dpage_pte_slot(pte);
		struct dpage_desc d;
		if (dpage_lookup(slot, &d) == 0) {
			if (d.src && d.copy_len > 0)
				memcpy((uint8_t *)frame + d.copy_off,
				       d.src, d.copy_len);
			dpage_free(slot);
		}
	}
	/* zero-on-demand: frame is already zero from memset above */

	/* Install a present PTE */
	uint64_t hhdm = vmm_get_hhdm();
	uint64_t phys  = (uint64_t)frame - hhdm;
	vmm_write_pte_in(pml4_phys, page_addr, phys | page_vmflags);

	klog(KLOG_DEBUG, "dpage_pf: faulted in 0x%lx (%s)\n",
	     (unsigned long)fault_addr,
	     dpage_pte_is_zero(pte) ? "zero" : "ELF");

	return 1;
}
