#ifndef _DICRON_PANIC_H
#define _DICRON_PANIC_H

void kpanic(const char *fmt, ...) __attribute__((noreturn, format(printf, 1, 2)));

#define KASSERT(expr)							\
	do {								\
		if (!(expr))						\
			kpanic("ASSERT FAILED: %s (%s:%d)\n",		\
			       #expr, __FILE__, __LINE__);		\
	} while (0)

#endif
