#ifndef _DICRON_CONSOLE_CONSOLE_H
#define _DICRON_CONSOLE_CONSOLE_H

#include <stddef.h>
#include <generated/autoconf.h>

struct limine_framebuffer;

#ifdef CONFIG_FRAMEBUFFER
void console_init(struct limine_framebuffer *fb);
void console_putchar(char c);
void console_write(const char *s, size_t len);
void console_clear(void);
void console_flush(void);
#else
static inline void console_init(struct limine_framebuffer *fb) { (void)fb; }
static inline void console_putchar(char c) { (void)c; }
static inline void console_write(const char *s, size_t len) { (void)s; (void)len; }
static inline void console_clear(void) {}
static inline void console_flush(void) {}
#endif

#endif
