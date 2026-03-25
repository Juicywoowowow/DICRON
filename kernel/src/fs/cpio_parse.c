#include <dicron/cpio.h>
#include "lib/string.h"

static uint32_t hex8_to_u32(const char *s)
{
	uint32_t val = 0;
	for (int i = 0; i < 8; i++) {
		char c = s[i];
		uint32_t digit;
		if (c >= '0' && c <= '9')
			digit = (uint32_t)(c - '0');
		else if (c >= 'a' && c <= 'f')
			digit = (uint32_t)(c - 'a') + 10;
		else if (c >= 'A' && c <= 'F')
			digit = (uint32_t)(c - 'A') + 10;
		else
			return 0;
		val = (val << 4) | digit;
	}
	return val;
}

static size_t align4(size_t v)
{
	return (v + 3) & ~(size_t)3;
}

size_t cpio_parse_entry(const void *archive, size_t archive_size,
			size_t offset, struct cpio_entry *entry)
{
	if (offset + CPIO_HEADER_SIZE > archive_size)
		return 0;

	const struct cpio_header *hdr =
		(const struct cpio_header *)((const char *)archive + offset);

	if (memcmp(hdr->magic, CPIO_MAGIC, CPIO_MAGIC_LEN) != 0)
		return 0;

	entry->mode = hex8_to_u32(hdr->mode);
	entry->filesize = hex8_to_u32(hdr->filesize);
	entry->namesize = hex8_to_u32(hdr->namesize);

	if (entry->namesize == 0)
		return 0;

	size_t name_offset = offset + CPIO_HEADER_SIZE;
	if (name_offset + entry->namesize > archive_size)
		return 0;

	entry->name = (const char *)archive + name_offset;

	/* Check for trailer */
	if (entry->namesize == 11 &&
	    memcmp(entry->name, CPIO_TRAILER, 10) == 0)
		return 0;

	size_t data_offset = align4(name_offset + entry->namesize);
	if (entry->filesize > 0) {
		if (data_offset + entry->filesize > archive_size)
			return 0;
		entry->data = (const char *)archive + data_offset;
	} else {
		entry->data = NULL;
	}

	return align4(data_offset + entry->filesize);
}
