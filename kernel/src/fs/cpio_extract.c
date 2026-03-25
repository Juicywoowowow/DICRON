#include <dicron/cpio.h>
#include <dicron/fs.h>
#include <dicron/log.h>
#include "lib/string.h"

int cpio_extract_all(const void *archive, size_t archive_size)
{
	if (!archive || archive_size < CPIO_HEADER_SIZE) return -1;

	size_t offset = 0;
	int files = 0;
	int dirs = 0;

	while (offset < archive_size) {
		struct cpio_entry entry;
		size_t next = cpio_parse_entry(archive, archive_size,
					       offset, &entry);
		if (next == 0) break;

		/* Skip the "." root entry */
		if (entry.namesize == 2 && entry.name[0] == '.') {
			offset = next;
			continue;
		}

		/* Build absolute path */
		char path[256];
		if (entry.name[0] == '/') {
			size_t len = strlen(entry.name);
			if (len >= sizeof(path)) { offset = next; continue; }
			memcpy(path, entry.name, len + 1);
		} else {
			path[0] = '/';
			size_t len = strlen(entry.name);
			if (len + 1 >= sizeof(path)) { offset = next; continue; }
			memcpy(path + 1, entry.name, len + 1);
		}

		/* Strip trailing slashes */
		size_t plen = strlen(path);
		while (plen > 1 && path[plen - 1] == '/') {
			path[--plen] = '\0';
		}

		if (S_ISDIR(entry.mode)) {
			cpio_mkdirs(path);
			dirs++;
		} else if (S_ISREG(entry.mode)) {
			int mode = (int)(entry.mode & 0777);
			if (cpio_write_file(path, entry.data,
					    entry.filesize, mode) == 0)
				files++;
			else
				klog(KLOG_WARN, "cpio: failed to extract '%s'\n",
				     path);
		}

		offset = next;
	}

	klog(KLOG_INFO, "cpio: extracted %d files, %d directories\n",
	     files, dirs);
	return 0;
}
