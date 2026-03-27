#ifndef _DICRON_ACPI_H
#define _DICRON_ACPI_H

#include <stdint.h>
#include <stddef.h>

/* Initialize ACPI parser with the physical/virtual address of the RSDP and the HHDM offset */
void acpi_init(void *rsdp, uint64_t hhdm_offset);

/* Find an ACPI table by its 4-character signature. Returns a virtual address or NULL. */
void *acpi_find_table(const char *signature);

/* Get the HHDM offset currently used for ACPI addresses */
uint64_t acpi_get_hhdm(void);

#endif
