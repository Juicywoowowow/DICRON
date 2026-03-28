#ifndef _DICRON_MM_VMM_H
#define _DICRON_MM_VMM_H

#include <stdint.h>

/* page table flags */
#define VMM_PRESENT	(1ULL << 0)
#define VMM_WRITE	(1ULL << 1)
#define VMM_USER	(1ULL << 2)
#define VMM_NX		(1ULL << 63)

void     vmm_init(uint64_t hhdm_offset);
int      vmm_map_page(uint64_t virt, uint64_t phys, uint64_t flags);
void     vmm_unmap_page(uint64_t virt);
uint64_t vmm_virt_to_phys(uint64_t virt);

/* Per-process address space support */
uint64_t vmm_get_hhdm(void);
uint64_t vmm_create_user_pml4(void);
void     vmm_switch_to(uint64_t pml4_phys);
uint64_t vmm_get_cr3(void);
int      vmm_map_page_in(uint64_t pml4_phys, uint64_t virt, uint64_t phys, uint64_t flags);
uint64_t vmm_virt_to_phys_in(uint64_t pml4_phys, uint64_t virt);
void     vmm_unmap_page_in(uint64_t pml4_phys, uint64_t virt);

/*
 * Raw PTE access — reads/writes page table entries without interpreting them.
 * Used by the swap subsystem to encode/decode swap slots in not-present PTEs.
 */
uint64_t vmm_read_pte_in(uint64_t pml4_phys, uint64_t virt);
void     vmm_write_pte_in(uint64_t pml4_phys, uint64_t virt, uint64_t raw_pte);

#endif
