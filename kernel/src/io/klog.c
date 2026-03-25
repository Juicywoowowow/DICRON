#include <dicron/log.h>
#include <dicron/io.h>
#include "lib/printf.h"
#include <stdarg.h>
#include <generated/autoconf.h>

#ifndef CONFIG_KLOG_LEVEL
#define CONFIG_KLOG_LEVEL KLOG_INFO
#endif

static int log_level = CONFIG_KLOG_LEVEL;

static const char *level_str[] = {
	"EMERG", "ERR", "WARN", "INFO", "DEBUG"
};

void klog(int level, const char *fmt, ...)
{
	if (level > log_level)
		return;

	char buf[1024];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	const char *tag = (level >= 0 && level <= KLOG_DEBUG)
			  ? level_str[level] : "???";

	kio_printf("[%s] %s", tag, buf);
}
