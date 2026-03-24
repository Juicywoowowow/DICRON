#ifndef _DICRON_DEV_H
#define _DICRON_DEV_H

#include <dicron/types.h>

#define KDEV_CHAR	0
#define KDEV_BLOCK	1

struct kdev;

/*
 * Device operations table.
 *
 * All callbacks return 0 / positive on success, negative errno on error.
 * read/write return bytes transferred (ssize_t) or negative errno.
 */
struct kdev_ops {
	int     (*open)(struct kdev *dev);
	int     (*close)(struct kdev *dev);
	ssize_t (*read)(struct kdev *dev, void *buf, size_t len);
	ssize_t (*write)(struct kdev *dev, const void *buf, size_t len);
	int     (*ioctl)(struct kdev *dev, unsigned long cmd, void *arg);
};

struct kdev {
	const char      *name;
	unsigned int     minor;
	int              type;
	struct kdev_ops *ops;
	void            *priv;
};

#endif
