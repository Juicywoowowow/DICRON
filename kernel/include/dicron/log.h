#ifndef _DICRON_LOG_H
#define _DICRON_LOG_H

#define KLOG_EMERG	0
#define KLOG_ERR	1
#define KLOG_WARN	2
#define KLOG_INFO	3
#define KLOG_DEBUG	4

void klog(int level, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

#endif
