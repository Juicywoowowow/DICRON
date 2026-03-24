#include "com.h"
#include "drivers/registry.h"
#include "arch/x86_64/io.h"
#include <dicron/dev.h>
#include <stddef.h>

#define COM1 0x3F8

static int serial_is_transmit_empty(void)
{
	return inb(COM1 + 5) & 0x20;
}

static void serial_putchar_raw(char c)
{
	while (!serial_is_transmit_empty())
		;
	outb(COM1, (uint8_t)c);
}

static ssize_t serial_write(struct kdev *dev, const void *buf, size_t len)
{
	(void)dev;
	const char *s = buf;

	for (size_t i = 0; i < len; i++) {
		if (s[i] == '\n')
			serial_putchar_raw('\r');
		serial_putchar_raw(s[i]);
	}
	return (ssize_t)len;
}

static struct kdev_ops serial_ops = {
	.open  = NULL,
	.close = NULL,
	.read  = NULL,
	.write = serial_write,
	.ioctl = NULL,
};

static struct kdev serial_dev = {
	.name  = "serial0",
	.minor = DRV_SERIAL,
	.type  = KDEV_CHAR,
	.ops   = &serial_ops,
	.priv  = NULL,
};

void serial_init(void)
{
	outb(COM1 + 1, 0x00);
	outb(COM1 + 3, 0x80);
	outb(COM1 + 0, 0x0C);
	outb(COM1 + 1, 0x00);
	outb(COM1 + 3, 0x03);
	outb(COM1 + 2, 0xC7);
	outb(COM1 + 4, 0x0B);

	int err = drv_register(DRV_SERIAL, &serial_dev);
	if (err)
		return;
}

int serial_getchar_nonblock(void)
{
	if (inb(COM1 + 5) & 0x01) {
		return inb(COM1);
	}
	return -1;
}
