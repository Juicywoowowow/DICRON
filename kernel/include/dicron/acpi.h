#ifndef _DICRON_ACPI_H
#define _DICRON_ACPI_H

#include <stdint.h>
#include <stddef.h>

/* Initialize ACPI parser with the physical/virtual address of the RSDP and the HHDM offset */

struct acpi_hpet_table {
	char signature[4];
	uint32_t length;
	uint8_t revision;
	uint8_t checksum;
	char oem_id[6];
	char oem_table_id[8];
	uint32_t oem_revision;
	uint32_t creator_id;
	uint32_t creator_revision;
	uint32_t event_timer_block_id;
	uint8_t base_address_id;
	uint8_t register_bit_width;
	uint8_t register_bit_offset;
	uint8_t access_width;
	uint64_t address;
	uint8_t hpet_number;
	uint16_t minimum_tick;
	uint8_t page_protection;
} __attribute__((packed));
void acpi_init(void *rsdp, uint64_t hhdm_offset);

/* Find an ACPI table by its 4-character signature. Returns a virtual address or NULL. */
void *acpi_find_table(const char *signature);

/* Get the HHDM offset currently used for ACPI addresses */
uint64_t acpi_get_hhdm(void);

#endif
