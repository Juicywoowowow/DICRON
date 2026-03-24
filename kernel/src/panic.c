#include <dicron/panic.h>
#include <dicron/io.h>
#include "lib/printf.h"
#include <stdarg.h>

void kpanic(const char *fmt, ...)
{
	__asm__ volatile("cli");

	char buf[512];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	kio_printf("\n!!! KERNEL PANIC !!!\n");
	kio_printf("%s", buf);
	kio_printf("System halted.\n");

	for (;;)
		__asm__ volatile("hlt");
}
