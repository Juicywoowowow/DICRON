#include "vmm.h"
#include "pmm.h"
#include "lib/string.h"
#include <dicron/panic.h>
#include <dicron/log.h>

static uint64_t hhdm;

/* read current PML4 base from CR3 */
static uint64_t read_cr3(void)
{
	uint64_t val;
	__asm__ volatile("mov %%cr3, %0" : "=r"(val));
	return val;
}

/* convert physical address to virtual via HHDM */
static uint64_t *phys_to_virt(uint64_t phys)
{
	return (uint64_t *)(hhdm + phys);
}

/* extract physical address from page table entry */
static uint64_t pte_addr(uint64_t entry)
{
	return entry & 0x000FFFFFFFFFF000ULL;
}

/* page table index helpers */
static unsigned int pml4_idx(uint64_t virt) { return (unsigned int)((virt >> 39) & 0x1FF); }
static unsigned int pdpt_idx(uint64_t virt) { return (unsigned int)((virt >> 30) & 0x1FF); }
static unsigned int pd_idx(uint64_t virt)   { return (unsigned int)((virt >> 21) & 0x1FF); }
static unsigned int pt_idx(uint64_t virt)   { return (unsigned int)((virt >> 12) & 0x1FF); }

/*
 * Walk the 4-level page table, creating intermediate tables as needed.
 * Returns a pointer to the final PTE.
 */
static uint64_t *vmm_walk(uint64_t virt, int create)
{
	uint64_t cr3 = read_cr3();
	uint64_t *pml4 = phys_to_virt(pte_addr(cr3));

	/* PML4 → PDPT */
	unsigned int i4 = pml4_idx(virt);
	if (!(pml4[i4] & VMM_PRESENT)) {
		if (!create)
			return NULL;
		void *page = pmm_alloc_page();
		if (!page)
			return NULL;
		memset(page, 0, PAGE_SIZE);
		uint64_t phys = (uint64_t)page - hhdm;
		pml4[i4] = phys | VMM_PRESENT | VMM_WRITE | VMM_USER;
	}
	uint64_t *pdpt = phys_to_virt(pte_addr(pml4[i4]));

	/* PDPT → PD */
	unsigned int i3 = pdpt_idx(virt);
	if (!(pdpt[i3] & VMM_PRESENT)) {
		if (!create)
			return NULL;
		void *page = pmm_alloc_page();
		if (!page)
			return NULL;
		memset(page, 0, PAGE_SIZE);
		uint64_t phys = (uint64_t)page - hhdm;
		pdpt[i3] = phys | VMM_PRESENT | VMM_WRITE | VMM_USER;
	}
	uint64_t *pd = phys_to_virt(pte_addr(pdpt[i3]));

	/* PD → PT */
	unsigned int i2 = pd_idx(virt);
	if (!(pd[i2] & VMM_PRESENT)) {
		if (!create)
			return NULL;
		void *page = pmm_alloc_page();
		if (!page)
			return NULL;
		memset(page, 0, PAGE_SIZE);
		uint64_t phys = (uint64_t)page - hhdm;
		pd[i2] = phys | VMM_PRESENT | VMM_WRITE | VMM_USER;
	}
	uint64_t *pt = phys_to_virt(pte_addr(pd[i2]));

	return &pt[pt_idx(virt)];
}

void vmm_init(uint64_t hhdm_offset)
{
	hhdm = hhdm_offset;
}

int vmm_map_page(uint64_t virt, uint64_t phys, uint64_t flags)
{
	uint64_t *pte = vmm_walk(virt, 1);
	if (!pte) {
		klog(KLOG_ERR, "vmm_map_page: failed to walk/create page tables for 0x%lx (OOM)\n", virt);
		return -1;
	}
	*pte = (phys & 0x000FFFFFFFFFF000ULL) | flags;
	__asm__ volatile("invlpg (%0)" : : "r"(virt) : "memory");
	return 0;
}

void vmm_unmap_page(uint64_t virt)
{
	uint64_t *pte = vmm_walk(virt, 0);
	if (pte) {
		*pte = 0;
		__asm__ volatile("invlpg (%0)" : : "r"(virt) : "memory");
	}
}

uint64_t vmm_virt_to_phys(uint64_t virt)
{
	uint64_t *pte = vmm_walk(virt, 0);
	if (!pte || !(*pte & VMM_PRESENT))
		return 0;
	return pte_addr(*pte) | (virt & 0xFFF);
}

uint64_t vmm_get_hhdm(void)
{
	return hhdm;
}

/*
 * Create a new PML4 for a user process.
 * Copies kernel-half entries (PML4[256..511]) so kernel is mapped everywhere.
 * User-half entries (PML4[0..255]) are empty.
 * Returns the PHYSICAL address of the new PML4.
 */
uint64_t vmm_create_user_pml4(void)
{
	void *page = pmm_alloc_page();
	if (!page)
		return 0;
	memset(page, 0, PAGE_SIZE);

	uint64_t *new_pml4 = (uint64_t *)page;
	uint64_t *kernel_pml4 = phys_to_virt(pte_addr(read_cr3()));

	/* Share all kernel-half entries */
	for (int i = 256; i < 512; i++)
		new_pml4[i] = kernel_pml4[i];

	uint64_t phys = (uint64_t)page - hhdm;
	return phys;
}

/* Switch to a different address space */
void vmm_switch_to(uint64_t pml4_phys)
{
	__asm__ volatile("mov %0, %%cr3" : : "r"(pml4_phys) : "memory");
}

/* Get the current CR3 */
uint64_t vmm_get_cr3(void)
{
	return pte_addr(read_cr3());
}

/*
 * Walk page tables rooted at a specific PML4 (by physical address).
 * Same as vmm_walk but operates on an arbitrary address space.
 */
static uint64_t *vmm_walk_at(uint64_t pml4_phys, uint64_t virt, int create)
{
	uint64_t *pml4 = phys_to_virt(pml4_phys);

	unsigned int i4 = pml4_idx(virt);
	if (!(pml4[i4] & VMM_PRESENT)) {
		if (!create) return NULL;
		void *page = pmm_alloc_page();
		if (!page) return NULL;
		memset(page, 0, PAGE_SIZE);
		pml4[i4] = ((uint64_t)page - hhdm) | VMM_PRESENT | VMM_WRITE | VMM_USER;
	}
	uint64_t *pdpt = phys_to_virt(pte_addr(pml4[i4]));

	unsigned int i3 = pdpt_idx(virt);
	if (!(pdpt[i3] & VMM_PRESENT)) {
		if (!create) return NULL;
		void *page = pmm_alloc_page();
		if (!page) return NULL;
		memset(page, 0, PAGE_SIZE);
		pdpt[i3] = ((uint64_t)page - hhdm) | VMM_PRESENT | VMM_WRITE | VMM_USER;
	}
	uint64_t *pd = phys_to_virt(pte_addr(pdpt[i3]));

	unsigned int i2 = pd_idx(virt);
	if (!(pd[i2] & VMM_PRESENT)) {
		if (!create) return NULL;
		void *page = pmm_alloc_page();
		if (!page) return NULL;
		memset(page, 0, PAGE_SIZE);
		pd[i2] = ((uint64_t)page - hhdm) | VMM_PRESENT | VMM_WRITE | VMM_USER;
	}
	uint64_t *pt = phys_to_virt(pte_addr(pd[i2]));

	return &pt[pt_idx(virt)];
}

/* Map a page into a specific address space */
int vmm_map_page_in(uint64_t pml4_phys, uint64_t virt, uint64_t phys, uint64_t flags)
{
	uint64_t *pte = vmm_walk_at(pml4_phys, virt, 1);
	if (!pte) {
		klog(KLOG_ERR, "vmm_map_page_in: OOM for 0x%lx\n", virt);
		return -1;
	}
	*pte = (phys & 0x000FFFFFFFFFF000ULL) | flags;
	return 0;
}

/* Translate virtual to physical in a specific address space */
uint64_t vmm_virt_to_phys_in(uint64_t pml4_phys, uint64_t virt)
{
	uint64_t *pte = vmm_walk_at(pml4_phys, virt, 0);
	if (!pte || !(*pte & VMM_PRESENT))
		return 0;
	return pte_addr(*pte) | (virt & 0xFFF);
}

/* Unmap a page in a specific address space */
void vmm_unmap_page_in(uint64_t pml4_phys, uint64_t virt)
{
	uint64_t *pte = vmm_walk_at(pml4_phys, virt, 0);
	if (pte)
		*pte = 0;
}
