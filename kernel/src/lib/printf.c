#include "printf.h"
#include "string.h"
#include <stdint.h>

static void put(char *buf, size_t size, size_t *pos, char c)
{
	if (*pos < size - 1)
		buf[*pos] = c;
	(*pos)++;
}

static void puts_n(char *buf, size_t size, size_t *pos, const char *s)
{
	while (*s)
		put(buf, size, pos, *s++);
}

static void put_uint(char *buf, size_t size, size_t *pos,
		     uint64_t val, int base, int width, char pad,
		     int uppercase)
{
	char tmp[20];
	const char *digits = uppercase ? "0123456789ABCDEF"
				       : "0123456789abcdef";
	int i = 0;

	if (val == 0)
		tmp[i++] = '0';
	else
		while (val) {
			tmp[i++] = digits[val % (uint64_t)base];
			val /= (uint64_t)base;
		}

	while (i < width)
		tmp[i++] = pad;

	while (i--)
		put(buf, size, pos, tmp[i]);
}

static void put_int(char *buf, size_t size, size_t *pos,
		    int64_t val, int width, char pad)
{
	if (val < 0) {
		put(buf, size, pos, '-');
		put_uint(buf, size, pos, (uint64_t)-val, 10, width, pad, 0);
	} else {
		put_uint(buf, size, pos, (uint64_t)val, 10, width, pad, 0);
	}
}

int vsnprintf(char *buf, size_t size, const char *fmt, va_list ap)
{
	size_t pos = 0;

	if (size == 0)
		return 0;

	while (*fmt) {
		if (*fmt != '%') {
			put(buf, size, &pos, *fmt++);
			continue;
		}
		fmt++; /* skip '%' */

		/* flags */
		char pad = ' ';
		
		while (*fmt == '0' || *fmt == '-') {
			if (*fmt == '0')
				pad = '0';
			/* - flag ignored for now, but prevents vararg corruption */
			fmt++;
		}

		/* width */
		int width = 0;
		while (*fmt >= '0' && *fmt <= '9')
			width = width * 10 + (*fmt++ - '0');

		/* length */
		int is_long = 0;
		if (*fmt == 'l') {
			is_long = 1;
			fmt++;
			if (*fmt == 'l') {
				fmt++;
			}
		}

		switch (*fmt) {
		case 'd':
		case 'i':
			if (is_long)
				put_int(buf, size, &pos,
					va_arg(ap, int64_t), width, pad);
			else
				put_int(buf, size, &pos,
					va_arg(ap, int), width, pad);
			break;
		case 'u':
			if (is_long)
				put_uint(buf, size, &pos,
					 va_arg(ap, uint64_t), 10, width,
					 pad, 0);
			else
				put_uint(buf, size, &pos,
					 va_arg(ap, unsigned int), 10, width,
					 pad, 0);
			break;
		case 'x':
			if (is_long)
				put_uint(buf, size, &pos,
					 va_arg(ap, uint64_t), 16, width,
					 pad, 0);
			else
				put_uint(buf, size, &pos,
					 va_arg(ap, unsigned int), 16, width,
					 pad, 0);
			break;
		case 'X':
			if (is_long)
				put_uint(buf, size, &pos,
					 va_arg(ap, uint64_t), 16, width,
					 pad, 1);
			else
				put_uint(buf, size, &pos,
					 va_arg(ap, unsigned int), 16, width,
					 pad, 1);
			break;
		case 'p': {
			uint64_t ptr = (uint64_t)va_arg(ap, void *);
			puts_n(buf, size, &pos, "0x");
			put_uint(buf, size, &pos, ptr, 16, 16, '0', 0);
			break;
		}
		case 's': {
			const char *s = va_arg(ap, const char *);
			if (!s)
				s = "(null)";
			puts_n(buf, size, &pos, s);
			break;
		}
		case 'c':
			put(buf, size, &pos, (char)va_arg(ap, int));
			break;
		case '%':
			put(buf, size, &pos, '%');
			break;
		default:
			put(buf, size, &pos, '%');
			put(buf, size, &pos, *fmt);
			break;
		}
		fmt++;
	}

	buf[pos < size ? pos : size - 1] = '\0';
	return (int)pos;
}

int snprintf(char *buf, size_t size, const char *fmt, ...)
{
	va_list ap;
	int ret;

	va_start(ap, fmt);
	ret = vsnprintf(buf, size, fmt, ap);
	va_end(ap);
	return ret;
}
