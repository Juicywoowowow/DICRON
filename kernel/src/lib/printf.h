#ifndef _DICRON_LIB_PRINTF_H
#define _DICRON_LIB_PRINTF_H

#include <stddef.h>
#include <stdarg.h>

int vsnprintf(char *buf, size_t size, const char *fmt, va_list ap);
int snprintf(char *buf, size_t size, const char *fmt, ...);

#endif
