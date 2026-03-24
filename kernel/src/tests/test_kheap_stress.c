#include "ktest.h"
#include "mm/kheap.h"
#include "lib/string.h"
#include <stdint.h>

KTEST_REGISTER(ktest_kheap_stress, "kmalloc stress", KTEST_CAT_BOOT)
static void ktest_kheap_stress(void)
{
	KTEST_BEGIN("kmalloc stress (256 mixed allocs)");

	#define STRESS_N 256
	void *ptrs[STRESS_N];
	size_t sizes[STRESS_N];

	for (int i = 0; i < STRESS_N; i++) {
		sizes[i] = (size_t)((i * 37) % 4000 + 8);
		ptrs[i] = kmalloc(sizes[i]);
		KTEST_NOT_NULL(ptrs[i], "stress alloc");
		memset(ptrs[i], (int)(i & 0xFF), sizes[i]);
	}

	int ok = 1;

	/* verify + free evens */
	for (int i = 0; i < STRESS_N; i += 2) {
		uint8_t *p = ptrs[i];
		for (size_t j = 0; j < sizes[i]; j++)
			if (p[j] != (uint8_t)(i & 0xFF)) ok = 0;
		kfree(ptrs[i]);
	}

	/* verify + free odds */
	for (int i = 1; i < STRESS_N; i += 2) {
		uint8_t *p = ptrs[i];
		for (size_t j = 0; j < sizes[i]; j++)
			if (p[j] != (uint8_t)(i & 0xFF)) ok = 0;
		kfree(ptrs[i]);
	}

	KTEST_TRUE(ok, "all 256 patterns intact after out-of-order free");
	#undef STRESS_N
}
