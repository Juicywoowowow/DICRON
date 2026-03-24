#include "registry.h"
#include <dicron/errno.h>
#include <stddef.h>

static struct kdev *devices[DRV_MAX];

int drv_register(unsigned int minor, struct kdev *dev)
{
	if (minor >= DRV_MAX || !dev)
		return -EINVAL;
	if (devices[minor])
		return -EBUSY;

	devices[minor] = dev;
	return 0;
}

int drv_unregister(unsigned int minor)
{
	if (minor >= DRV_MAX)
		return -EINVAL;

	devices[minor] = NULL;
	return 0;
}

struct kdev *drv_get(unsigned int minor)
{
	if (minor >= DRV_MAX)
		return NULL;
	return devices[minor];
}
