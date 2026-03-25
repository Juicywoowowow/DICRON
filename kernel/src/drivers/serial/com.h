#ifndef _DICRON_DRIVERS_SERIAL_COM_H
#define _DICRON_DRIVERS_SERIAL_COM_H

#include <generated/autoconf.h>

#ifdef CONFIG_SERIAL
void serial_init(void);
int  serial_getchar_nonblock(void);
#else
static inline void serial_init(void) {}
static inline int serial_getchar_nonblock(void) { return -1; }
#endif

#endif
