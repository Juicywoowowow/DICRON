#include <dicron/io.h>
#include <dicron/dev.h>
#include "drivers/registry.h"
#include "drivers/ps2/kbd.h"
#include "drivers/serial/com.h"
#include "console/console.h"
#include "lib/printf.h"
#include <stdarg.h>
#include <dicron/spinlock.h>

static spinlock_t print_lock = SPINLOCK_INIT;

int kio_putchar(int c)
{
	char ch = (char)c;

	uint64_t flags = spin_lock_irqsave(&print_lock);
	console_putchar(ch);
	console_flush();

	struct kdev *serial = drv_get(DRV_SERIAL);
	if (serial && serial->ops->write)
		serial->ops->write(serial, &ch, 1);
	spin_unlock_irqrestore(&print_lock, flags);

	return c;
}

int kio_puts(const char *s)
{
	uint64_t flags = spin_lock_irqsave(&print_lock);

	struct kdev *serial = drv_get(DRV_SERIAL);
	int count = 0;

	while (*s) {
		console_putchar(*s);
		if (serial && serial->ops->write)
			serial->ops->write(serial, s, 1);
		s++;
		count++;
	}
	console_flush();
	spin_unlock_irqrestore(&print_lock, flags);
	return count;
}

int kio_printf(const char *fmt, ...)
{
	char buf[1024];
	va_list ap;

	va_start(ap, fmt);
	int len = vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	uint64_t flags = spin_lock_irqsave(&print_lock);

	struct kdev *serial = drv_get(DRV_SERIAL);
	for (int i = 0; i < len && buf[i]; i++) {
		console_putchar(buf[i]);
		if (serial && serial->ops->write)
			serial->ops->write(serial, &buf[i], 1);
	}
	console_flush();
	spin_unlock_irqrestore(&print_lock, flags);

	return len;
}

int kio_getchar(void)
{
	for (;;) {
		int c = kbd_getchar_nonblock();
		if (c != -1)
			return c;

		c = serial_getchar_nonblock();
		if (c != -1) {
			if (c == '\r')
				c = '\n';
			return c;
		}

		__asm__ volatile("hlt" ::: "memory");
	}
}
