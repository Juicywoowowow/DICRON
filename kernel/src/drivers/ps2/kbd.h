#ifndef _DICRON_DRIVERS_PS2_KBD_H
#define _DICRON_DRIVERS_PS2_KBD_H

#include <generated/autoconf.h>

#ifdef CONFIG_PS2_KBD
void kbd_init(void);
int  kbd_getchar(void); /* blocking */
int  kbd_getchar_nonblock(void);
#else
static inline void kbd_init(void) {}
static inline int kbd_getchar(void) {
	for(;;) __asm__ volatile("hlt" ::: "memory");
}
static inline int kbd_getchar_nonblock(void) { return -1; }
#endif

#endif
