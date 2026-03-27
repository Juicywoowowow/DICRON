#include <dicron/log.h>
#include <dicron/io.h>
#include "lib/printf.h"
#include <stdarg.h>
#include <generated/autoconf.h>

static const char *level_str[] = {
	"EMERG", "ERR", "WARN", "INFO", "DEBUG"
};

static int level_enabled(int level)
{
	switch (level) {
#ifdef CONFIG_KLOG_SHOW_EMERG
	case KLOG_EMERG: return 1;
#endif
#ifdef CONFIG_KLOG_SHOW_ERR
	case KLOG_ERR:   return 1;
#endif
#ifdef CONFIG_KLOG_SHOW_WARN
	case KLOG_WARN:  return 1;
#endif
#ifdef CONFIG_KLOG_SHOW_INFO
	case KLOG_INFO:  return 1;
#endif
#ifdef CONFIG_KLOG_SHOW_DEBUG
	case KLOG_DEBUG: return 1;
#endif
	default: return 0;
	}
}

void klog(int level, const char *fmt, ...)
{
	if (!level_enabled(level))
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
