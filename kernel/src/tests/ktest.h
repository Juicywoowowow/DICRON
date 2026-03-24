#ifndef _DICRON_TESTS_KTEST_H
#define _DICRON_TESTS_KTEST_H

#include <dicron/io.h>
#include <dicron/panic.h>
#include <stddef.h>
#include <stdint.h>

#define BOOT_TEST_SHOW 0

/* ── test categories ── */
#define KTEST_CAT_BOOT 0
#define KTEST_CAT_POST 1

/* ── auto-registration ── */
struct ktest_entry {
  const char *name;
  void (*fn)(void);
  int category;
} __attribute__((aligned(32)));

#define KTEST_REGISTER(func, suite_name, cat)                                  \
  static void func(void);                                                      \
  __attribute__((used,                                                         \
                 section(".ktest_registry"))) static const struct ktest_entry  \
      _ktest_entry_##func = {                                                  \
          .name = suite_name,                                                  \
          .fn = func,                                                          \
          .category = cat,                                                     \
  };

extern const struct ktest_entry __ktest_start[];
extern const struct ktest_entry __ktest_end[];

/* ── per-suite stats ── */
struct ktest_stats {
  int passed;
  int failed;
  int total;
};

extern struct ktest_stats ktest_stats;

/* ── suite begin ── */
#if BOOT_TEST_SHOW
#define KTEST_BEGIN(suite_name) kio_printf("\n--- %s ---\n", (suite_name))
#else
#define KTEST_BEGIN(suite_name)                                                \
  do {                                                                         \
  } while (0)
#endif

/* ── basic assertion ── */
#define KTEST(expr, name)                                                      \
  do {                                                                         \
    ktest_stats.total++;                                                       \
    if (expr) {                                                                \
      ktest_stats.passed++;                                                    \
    } else {                                                                   \
      ktest_stats.failed++;                                                    \
      kio_printf("  [FAIL] %s (%s:%d)\n", (name), __FILE__, __LINE__);         \
    }                                                                          \
  } while (0)

/* ── comparison assertions with actual/expected printing ── */
#define KTEST_EQ(a, b, name)                                                   \
  do {                                                                         \
    long long _a = (long long)(a);                                             \
    long long _b = (long long)(b);                                             \
    ktest_stats.total++;                                                       \
    if (_a == _b) {                                                            \
      ktest_stats.passed++;                                                    \
    } else {                                                                   \
      ktest_stats.failed++;                                                    \
      kio_printf("  [FAIL] %s (%s:%d)"                                         \
                 " — got %lld, expected %lld\n",                               \
                 (name), __FILE__, __LINE__, _a, _b);                          \
    }                                                                          \
  } while (0)

#define KTEST_NE(a, b, name)                                                   \
  do {                                                                         \
    long long _a = (long long)(a);                                             \
    long long _b = (long long)(b);                                             \
    ktest_stats.total++;                                                       \
    if (_a != _b) {                                                            \
      ktest_stats.passed++;                                                    \
    } else {                                                                   \
      ktest_stats.failed++;                                                    \
      kio_printf("  [FAIL] %s (%s:%d)"                                         \
                 " — both equal %lld\n",                                       \
                 (name), __FILE__, __LINE__, _a);                              \
    }                                                                          \
  } while (0)

#define KTEST_GT(a, b, name)                                                   \
  do {                                                                         \
    long long _a = (long long)(a);                                             \
    long long _b = (long long)(b);                                             \
    ktest_stats.total++;                                                       \
    if (_a > _b) {                                                             \
      ktest_stats.passed++;                                                    \
    } else {                                                                   \
      ktest_stats.failed++;                                                    \
      kio_printf("  [FAIL] %s (%s:%d)"                                         \
                 " — got %lld, expected > %lld\n",                             \
                 (name), __FILE__, __LINE__, _a, _b);                          \
    }                                                                          \
  } while (0)

#define KTEST_GE(a, b, name)                                                   \
  do {                                                                         \
    long long _a = (long long)(a);                                             \
    long long _b = (long long)(b);                                             \
    ktest_stats.total++;                                                       \
    if (_a >= _b) {                                                            \
      ktest_stats.passed++;                                                    \
    } else {                                                                   \
      ktest_stats.failed++;                                                    \
      kio_printf("  [FAIL] %s (%s:%d)"                                         \
                 " — got %lld, expected >= %lld\n",                            \
                 (name), __FILE__, __LINE__, _a, _b);                          \
    }                                                                          \
  } while (0)

#define KTEST_LT(a, b, name)                                                   \
  do {                                                                         \
    long long _a = (long long)(a);                                             \
    long long _b = (long long)(b);                                             \
    ktest_stats.total++;                                                       \
    if (_a < _b) {                                                             \
      ktest_stats.passed++;                                                    \
    } else {                                                                   \
      ktest_stats.failed++;                                                    \
      kio_printf("  [FAIL] %s (%s:%d)"                                         \
                 " — got %lld, expected < %lld\n",                             \
                 (name), __FILE__, __LINE__, _a, _b);                          \
    }                                                                          \
  } while (0)

#define KTEST_NOT_NULL(p, name)                                                \
  do {                                                                         \
    const void *_p = (const void *)(p);                                        \
    ktest_stats.total++;                                                       \
    if (_p != ((void *)0)) {                                                   \
      ktest_stats.passed++;                                                    \
    } else {                                                                   \
      ktest_stats.failed++;                                                    \
      kio_printf("  [FAIL] %s (%s:%d)"                                         \
                 " — pointer is NULL\n",                                       \
                 (name), __FILE__, __LINE__);                                  \
    }                                                                          \
  } while (0)

#define KTEST_NULL(p, name)                                                    \
  do {                                                                         \
    const void *_p = (const void *)(p);                                        \
    ktest_stats.total++;                                                       \
    if (_p == ((void *)0)) {                                                   \
      ktest_stats.passed++;                                                    \
    } else {                                                                   \
      ktest_stats.failed++;                                                    \
      kio_printf("  [FAIL] %s (%s:%d)"                                         \
                 " — pointer is %p, expected NULL\n",                          \
                 (name), __FILE__, __LINE__, _p);                              \
    }                                                                          \
  } while (0)

#define KTEST_TRUE(expr, name) KTEST(!!(expr), name)
#define KTEST_FALSE(expr, name) KTEST(!(expr), name)

/* ── runner API ── */
void ktest_init(void);
void ktest_summary(void);
int ktest_run_all(void);
int ktest_run_post(void);

#endif
