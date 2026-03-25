#ifndef _DICRON_CPIO_H
#define _DICRON_CPIO_H

#include <stdint.h>
#include <stddef.h>

#define CPIO_MAGIC "070701"
#define CPIO_MAGIC_LEN 6
#define CPIO_HEADER_SIZE 110
#define CPIO_TRAILER "TRAILER!!!"

struct cpio_header {
	char magic[6];
	char ino[8];
	char mode[8];
	char uid[8];
	char gid[8];
	char nlink[8];
	char mtime[8];
	char filesize[8];
	char devmajor[8];
	char devminor[8];
	char rdevmajor[8];
	char rdevminor[8];
	char namesize[8];
	char check[8];
};

struct cpio_entry {
	const char *name;
	const void *data;
	uint32_t mode;
	uint32_t filesize;
	uint32_t namesize;
};

/* Parse a single cpio entry at the given offset. Returns bytes consumed,
 * or 0 on TRAILER / error. Fills out `entry`. */
size_t cpio_parse_entry(const void *archive, size_t archive_size,
			size_t offset, struct cpio_entry *entry);

/* Extract an entire cpio newc archive into the VFS/ramfs. */
int cpio_extract_all(const void *archive, size_t archive_size);

/* Write a single cpio entry's data into a VFS file. */
int cpio_write_file(const char *path, const void *data, size_t size, int mode);

/* Create all directories in a path (like mkdir -p). */
int cpio_mkdirs(const char *path);

#endif
