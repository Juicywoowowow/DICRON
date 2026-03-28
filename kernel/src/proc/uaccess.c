#include <dicron/uaccess.h>
#include <dicron/log.h>
#include "lib/string.h"

/*
 * uaccess.c — Safe user↔kernel memory operations.
 *
 * Every syscall that takes a user pointer MUST validate it through
 * these functions before dereferencing. A malicious program could
 * otherwise read/write kernel memory.
 */

#include "mm/vmm.h"
#include <dicron/process.h>

#ifdef CONFIG_SWAP
#include "mm/swap.h"
/* Forward declare to avoid header dependency hell if it exists */
extern int swap_pf_handle(uint64_t fault_addr, uint64_t page_dir_phys);
extern int swap_pte_is_swap(uint64_t pte);
#endif

int uaccess_valid(const void *ptr, size_t len)
{
	uint64_t start = (uint64_t)ptr;
	uint64_t end = start + len;

	if (end < start) return 0;
	if (start == 0 && len > 0) return 0;
	if (end > USER_SPACE_END) return 0;

	struct process *proc = process_current();
	if (!proc) return 1;

	uint64_t cr3 = proc->cr3;
	uint64_t page = start & ~0xFFFULL;
	
	while (page < end) {
		uint64_t pte = vmm_read_pte_in(cr3, page);
		if (!(pte & VMM_PRESENT)) {
#ifdef CONFIG_SWAP
			if (swap_pte_is_swap(pte)) {
				if (!swap_pf_handle(page, cr3)) return 0;
				pte = vmm_read_pte_in(cr3, page);
			} else {
				return 0;
			}
#else
			return 0;
#endif
		}
		if (!(pte & VMM_USER)) return 0;
		page += 4096;
	}
	return 1;
}

int uaccess_valid_string(const char *str, size_t max_len)
{
	uint64_t addr = (uint64_t)str;

	if (addr == 0 || addr >= USER_SPACE_END) return 0;

	struct process *proc = process_current();
	uint64_t cr3 = proc ? proc->cr3 : 0;

	for (size_t i = 0; i < max_len; i++) {
		if (addr + i >= USER_SPACE_END) return 0;
		
		/* Check new page boundary */
		if (cr3 && ((addr + i) % 4096 == 0 || i == 0)) {
			uint64_t page = (addr + i) & ~0xFFFULL;
			uint64_t pte = vmm_read_pte_in(cr3, page);
			if (!(pte & VMM_PRESENT)) {
#ifdef CONFIG_SWAP
				if (swap_pte_is_swap(pte)) {
					if (!swap_pf_handle(page, cr3)) return 0;
					pte = vmm_read_pte_in(cr3, page);
				} else {
					return 0;
				}
#else
				return 0;
#endif
			}
			if (!(pte & VMM_USER)) return 0;
		}

		if (str[i] == '\0') return 1;
	}
	return 0;
}

int copy_from_user(void *dst, const void *user_src, size_t len)
{
	if (!uaccess_valid(user_src, len)) {
		klog(KLOG_ERR, "uaccess: bad copy_from_user ptr=%p len=%lu\n",
		     user_src, len);
		return -14; /* -EFAULT */
	}

	memcpy(dst, user_src, len);
	return 0;
}

int copy_to_user(void *user_dst, const void *src, size_t len)
{
	if (!uaccess_valid(user_dst, len)) {
		klog(KLOG_ERR, "uaccess: bad copy_to_user ptr=%p len=%lu\n",
		     user_dst, len);
		return -14; /* -EFAULT */
	}

	memcpy(user_dst, src, len);
	return 0;
}
