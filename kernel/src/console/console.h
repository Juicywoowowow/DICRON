#ifndef _DICRON_CONSOLE_CONSOLE_H
#define _DICRON_CONSOLE_CONSOLE_H

#include <stddef.h>

struct limine_framebuffer;

void console_init(struct limine_framebuffer *fb);
void console_putchar(char c);
void console_write(const char *s, size_t len);
void console_clear(void);
void console_flush(void);

#endif
