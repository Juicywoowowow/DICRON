#include "sata.h"
#include "drivers/pci/ahci/ahci.h"
#include "mm/pmm.h"
#include "lib/string.h"
#include <dicron/errno.h>
#include <dicron/log.h>
#include <generated/autoconf.h>

#ifdef CONFIG_SATA

static struct sata_device devices[SATA_MAX_DEVICES];
static int ndevs;

static void ata_string_copy(char *dst, const uint16_t *src,
			    int word_start, int word_count)
{
	for (int i = 0; i < word_count; i++) {
		uint16_t w = src[word_start + i];
		dst[i * 2] = (char)(w >> 8);
		dst[i * 2 + 1] = (char)(w & 0xFF);
	}
	dst[word_count * 2] = '\0';

	for (int i = word_count * 2 - 1; i >= 0 && dst[i] == ' '; i--)
		dst[i] = '\0';
}

int sata_detect_ports(void)
{
	int port_count = ahci_port_count();

	for (int i = 0; i < port_count && ndevs < SATA_MAX_DEVICES; i++) {
		struct ahci_port *port = ahci_get_port(i);
		if (!port)
			continue;

		void *id_buf = pmm_alloc_page();
		if (!id_buf)
			continue;
		memset(id_buf, 0, PAGE_SIZE);

		if (ahci_cmd_identify(port, id_buf) < 0) {
			pmm_free_page(id_buf);
			continue;
		}

		uint16_t *id = (uint16_t *)id_buf;
		struct sata_device *dev = &devices[ndevs];

		dev->present = 1;
		dev->port_index = i;

		dev->total_sectors = (uint64_t)id[100]
			| ((uint64_t)id[101] << 16)
			| ((uint64_t)id[102] << 32)
			| ((uint64_t)id[103] << 48);

		if (dev->total_sectors == 0)
			dev->total_sectors = (uint64_t)id[60]
				| ((uint64_t)id[61] << 16);

		ata_string_copy(dev->model, id, 27, 20);
		ata_string_copy(dev->serial, id, 10, 10);

		klog(KLOG_INFO, "sata: %s (%llu MB)\n",
		     dev->model,
		     (unsigned long long)(dev->total_sectors / 2048));

		pmm_free_page(id_buf);
		ndevs++;
	}

	return ndevs;
}

int sata_drive_count(void)
{
	return ndevs;
}

struct sata_device *sata_get_drive(int index)
{
	if (index < 0 || index >= ndevs)
		return NULL;
	return &devices[index];
}

#endif /* CONFIG_SATA */
