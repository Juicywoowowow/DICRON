#ifndef _DICRON_MM_DPAGE_H
#define _DICRON_MM_DPAGE_H

#include <stdint.h>
#include <stddef.h>

/*
 * Demand paging subsystem.
 *
 * Pages are not backed by physical memory at map time.  Instead, a
 * not-present PTE carries enough information to populate the page on
 * the first access (page fault).
 *
 * Two demand flavours:
 *   1. ELF-backed — copy bytes from an in-memory ELF image on fault.
 *   2. Zero-page  — return a zeroed frame on fault (anonymous mmap/brk).
 *
 * PTE encoding (all demand PTEs have Present bit = 0):
 *
 *   bit  0   = 0  — not present (CPU never interprets remaining bits)
 *   bit  1   = 0  — NOT a swap marker  (distinguishes from swap PTEs)
 *   bit  4   = 1  — DPAGE_PTE_MARKER: this PTE holds demand-paging info
 *   bit  5   = 1  — DPAGE_PTE_ZERO: zero-on-demand (no descriptor slot)
 *                   (only valid when bit 4 set; overrides slot field)
 *   bits 6:63    — DPAGE_PTE_SLOT: 58-bit descriptor slot index
 *                  (ignored when DPAGE_PTE_ZERO is set)
 *
 * The swap PTE uses bit 1 as its marker — demand PTEs leave bit 1 = 0
 * so the two encodings never collide.
 */

#define DPAGE_PTE_MARKER	(1ULL << 4)
#define DPAGE_PTE_ZERO		(1ULL << 5)
#define DPAGE_PTE_SLOT_SHIFT	6
#define DPAGE_PTE_SLOT_MASK	0x3FFFFFFFFFFFFFFULL

static inline uint64_t dpage_pte_encode_zero(void)
{
	return DPAGE_PTE_MARKER | DPAGE_PTE_ZERO;
}

static inline uint64_t dpage_pte_encode_slot(uint64_t slot)
{
	return DPAGE_PTE_MARKER | (slot << DPAGE_PTE_SLOT_SHIFT);
}

static inline int dpage_pte_is_demand(uint64_t pte)
{
	/* Present = 0, swap marker = 0 (bit 1), demand marker = 1 (bit 4) */
	return !(pte & 1ULL) && !(pte & (1ULL << 1)) && !!(pte & DPAGE_PTE_MARKER);
}

static inline int dpage_pte_is_zero(uint64_t pte)
{
	return !!(pte & DPAGE_PTE_ZERO);
}

static inline uint64_t dpage_pte_slot(uint64_t pte)
{
	return (pte >> DPAGE_PTE_SLOT_SHIFT) & DPAGE_PTE_SLOT_MASK;
}

/* ------------------------------------------------------------------ */
/*  Descriptor table                                                   */
/* ------------------------------------------------------------------ */

/*
 * A demand descriptor stores enough info to populate one page on fault.
 * For ELF-backed pages: pointer into the loaded ELF image, file offset,
 * and byte counts for the mapped portion vs. zero-fill tail.
 *
 * All pointers are kernel virtual (HHDM) addresses valid at fault time
 * because the ELF data lives in the initrd / kernel heap until the
 * process exits.
 */
struct dpage_desc {
	const uint8_t *src;	/* pointer to source bytes in ELF image */
	uint16_t       copy_off;	/* offset within page to start copy  */
	uint16_t       copy_len;	/* number of bytes to copy from src  */
	/* remainder of page [copy_off+copy_len .. 4095] is zeroed */
};

#define DPAGE_MAX_SLOTS	512

/*
 * Allocate a demand descriptor slot.  Fills *desc_out and returns the
 * slot index (>= 0) on success, -1 when the table is full.
 */
int dpage_alloc(const struct dpage_desc *desc);

/*
 * Look up a descriptor by slot index.
 * Returns 0 on success, -1 if slot is out of range or unused.
 */
int dpage_lookup(uint64_t slot, struct dpage_desc *desc_out);

/*
 * Free a descriptor slot (called after fault-time population).
 */
void dpage_free(uint64_t slot);

/*
 * Handle a demand-paging page fault.
 *
 * Called from the page-fault handler with interrupts disabled.
 * Inspects the raw PTE at fault_addr in the given address space.
 * If it carries a demand marker:
 *   - Allocates a physical frame
 *   - Copies/zeros the appropriate bytes
 *   - Installs a present PTE and frees the descriptor slot
 *   - Returns 1 (fault handled; instruction will be retried)
 * Returns 0 if the PTE is not a demand PTE.
 */
int dpage_pf_handle(uint64_t fault_addr, uint64_t pml4_phys,
		    uint64_t page_vmflags);

#endif /* _DICRON_MM_DPAGE_H */
