#ifndef FUZZYGEN_H
#define FUZZYGEN_H

#define FUZZ_MAX_TESTS   100
#define FUZZ_MIN_TESTS   1
#define FUZZ_OUTPUT_DIR  "../kernel/src/tests"

/* Fuzz scenario types */
enum fuzz_category {
	FUZZ_PMM,
	FUZZ_SLAB,
	FUZZ_KHEAP,
	FUZZ_STRING,
	FUZZ_MATH,
	FUZZ_SPINLOCK,
	FUZZ_CATEGORY_COUNT
};

static const char *fuzz_category_names[] = {
	"pmm", "slab", "kheap", "string", "math", "spinlock"
};

#endif
