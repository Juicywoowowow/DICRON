#include "lib/string.h"
#include <dicron/acpi.h>
#include <dicron/log.h>
#include <dicron/panic.h>
#include <stddef.h>
#include <stdint.h>
#include "mm/vmm.h"

struct acpi_rsdp {
  char signature[8];
  uint8_t checksum;
  char oem_id[6];
  uint8_t revision;
  uint32_t rsdt_address;
} __attribute__((packed));

struct acpi_rsdp2 {
  struct acpi_rsdp first_part;
  uint32_t length;
  uint64_t xsdt_address;
  uint8_t ext_checksum;
  uint8_t reserved[3];
} __attribute__((packed));

struct acpi_sdt_hdr {
  char signature[4];
  uint32_t length;
  uint8_t revision;
  uint8_t checksum;
  char oem_id[6];
  char oem_table_id[8];
  uint32_t oem_revision;
  uint32_t creator_id;
  uint32_t creator_revision;
} __attribute__((packed));

static void *acpi_sdt_base = NULL;
static int acpi_use_xsdt = 0;
static uint64_t hhdm = 0;

static uint8_t acpi_checksum(const void *base, size_t len) {
  const uint8_t *p = base;
  uint8_t sum = 0;
  for (size_t i = 0; i < len; i++) {
    sum += p[i];
  }
  return sum;
}

static inline void *to_virt(uint64_t addr) {
	if (!addr) return NULL;
	if (addr < 0xFFFF800000000000ULL) {
		uint64_t virt = addr + hhdm;
		/*
		 * Limine's HHDM may not cover ACPI regions (reclaim memory,
		 * reserved areas, or anything below 1 MB).  Ensure the page
		 * (and the next one, in case the structure straddles a
		 * boundary) is mapped so we can read the table.
		 */
		uint64_t phys_page = addr & ~0xFFFULL;
		uint64_t virt_page = virt & ~0xFFFULL;
		vmm_map_page(virt_page, phys_page, VMM_PRESENT | VMM_WRITE);
		vmm_map_page(virt_page + 0x1000, phys_page + 0x1000, VMM_PRESENT | VMM_WRITE);
		return (void *)virt;
	}
	return (void *)addr;
}

void acpi_init(void *rsdp_ptr, uint64_t hhdm_offset) {
  hhdm = hhdm_offset;
  if (!rsdp_ptr) {
    klog(KLOG_WARN, "ACPI: No RSDP provided\n");
    return;
  }

  struct acpi_rsdp *rsdp = (struct acpi_rsdp *)to_virt((uint64_t)rsdp_ptr);
  if (strncmp(rsdp->signature, "RSD PTR ", 8) != 0) {
    klog(KLOG_ERR, "ACPI: Invalid RSDP signature\n");
    return;
  }

  if (acpi_checksum(rsdp, sizeof(struct acpi_rsdp)) != 0) {
    klog(KLOG_ERR, "ACPI: Invalid RSDP checksum\n");
    return;
  }

  if (rsdp->revision == 0) {
    acpi_use_xsdt = 0;
    acpi_sdt_base = to_virt(rsdp->rsdt_address);
  } else {
    struct acpi_rsdp2 *rsdp2 = (struct acpi_rsdp2 *)rsdp;
    if (acpi_checksum(rsdp2, rsdp2->length) != 0) {
      klog(KLOG_ERR, "ACPI: Invalid RSDP v2 checksum\n");
      return;
    }
    acpi_use_xsdt = 1;
    acpi_sdt_base = to_virt(rsdp2->xsdt_address);
  }

  struct acpi_sdt_hdr *hdr = acpi_sdt_base;
  if (acpi_checksum(hdr, hdr->length) != 0) {
    klog(KLOG_ERR, "ACPI: Invalid %s checksum\n",
         acpi_use_xsdt ? "XSDT" : "RSDT");
    acpi_sdt_base = NULL;
    return;
  }

  klog(KLOG_INFO, "ACPI: Initialized (%s)\n", acpi_use_xsdt ? "XSDT" : "RSDT");
}

void *acpi_find_table(const char *signature) {
  if (!acpi_sdt_base)
    return NULL;

  struct acpi_sdt_hdr *sdt = acpi_sdt_base;
  int entries = (int)((sdt->length - sizeof(struct acpi_sdt_hdr)) /
                      (acpi_use_xsdt ? 8 : 4));

  for (int i = 0; i < entries; i++) {
    struct acpi_sdt_hdr *hdr;
    if (acpi_use_xsdt) {
      uint64_t *ptr =
          (uint64_t *)((uint8_t *)sdt + sizeof(struct acpi_sdt_hdr));
      hdr = (struct acpi_sdt_hdr *)to_virt(ptr[i]);
    } else {
      uint32_t *ptr =
          (uint32_t *)((uint8_t *)sdt + sizeof(struct acpi_sdt_hdr));
      hdr = (struct acpi_sdt_hdr *)to_virt(ptr[i]);
    }

    if (strncmp(hdr->signature, signature, 4) == 0) {
      if (acpi_checksum(hdr, hdr->length) == 0)
        return hdr;
      else
        klog(KLOG_WARN, "ACPI: Found %s but invalid checksum\n", signature);
    }
  }
  return NULL;
}

uint64_t acpi_get_hhdm(void)
{
	return hhdm;
}
