#include <dicron/cpio.h>
#include <dicron/fs.h>
#include <dicron/mem.h>
#include <dicron/log.h>
#include "lib/string.h"

int cpio_mkdirs(const char *path)
{
	if (!path || *path != '/') return -1;

	char buf[256];
	size_t len = strlen(path);
	if (len >= sizeof(buf)) return -1;
	memcpy(buf, path, len + 1);

	for (size_t i = 1; i <= len; i++) {
		if (buf[i] == '/' || buf[i] == '\0') {
			char saved = buf[i];
			buf[i] = '\0';

			struct inode *check = vfs_namei(buf);
			if (!check)
				vfs_mkdir(buf, 0755);

			buf[i] = saved;
			if (saved == '\0') break;
		}
	}
	return 0;
}

int cpio_write_file(const char *path, const void *data, size_t size, int mode)
{
	/* Ensure parent directories exist */
	char parent[256];
	size_t plen = strlen(path);
	if (plen >= sizeof(parent)) return -1;
	memcpy(parent, path, plen + 1);

	/* Find last slash to get parent dir */
	char *last_slash = NULL;
	for (size_t i = 0; i < plen; i++) {
		if (parent[i] == '/')
			last_slash = &parent[i];
	}
	if (last_slash && last_slash != parent) {
		*last_slash = '\0';
		cpio_mkdirs(parent);
	}

	/* Create the file */
	if (vfs_create(path, mode) != 0) return -1;

	/* Write data if any */
	if (size > 0 && data) {
		struct inode *node = vfs_namei(path);
		if (!node || !node->f_op || !node->f_op->write) return -1;

		struct file f = {
			.f_inode = node,
			.f_op = node->f_op,
			.f_pos = 0,
		};
		long written = node->f_op->write(&f, (const char *)data, size);
		if (written < 0 || (size_t)written != size) return -1;
	}

	return 0;
}
