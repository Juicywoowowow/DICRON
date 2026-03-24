#ifndef _DICRON_IO_H
#define _DICRON_IO_H

int kio_putchar(int c);
int kio_puts(const char *s);
int kio_printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
int kio_getchar(void);

#endif
