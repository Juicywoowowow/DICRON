#ifndef _DICRON_DRIVERS_PS2_KBD_H
#define _DICRON_DRIVERS_PS2_KBD_H

void kbd_init(void);
int  kbd_getchar(void); /* blocking */
int  kbd_getchar_nonblock(void);

#endif
