#ifndef _DICRON_DRIVERS_REGISTRY_H
#define _DICRON_DRIVERS_REGISTRY_H

#include <dicron/dev.h>

#define DRV_MAX		64

/* well-known driver IDs */
#define DRV_SERIAL	0
#define DRV_KBD		1

int          drv_register(unsigned int minor, struct kdev *dev);
int          drv_unregister(unsigned int minor);
struct kdev *drv_get(unsigned int minor);

#endif
