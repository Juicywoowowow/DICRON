#include "ktest.h"
#include <generated/autoconf.h>

struct ktest_stats ktest_stats;

void ktest_init(void) {
  ktest_stats.passed = 0;
  ktest_stats.failed = 0;
  ktest_stats.total = 0;
}

void ktest_summary(void) {
#ifdef CONFIG_TESTS_SHOW_VERBOSE
  kio_printf("\n========================================\n");
  kio_printf("  BOOT TEST SUMMARY\n");
  kio_printf("  Total:  %d\n", ktest_stats.total);
  kio_printf("  Passed: %d\n", ktest_stats.passed);
  kio_printf("  Failed: %d\n", ktest_stats.failed);
  kio_printf("========================================\n");
#endif

  if (ktest_stats.failed > 0)
    kio_printf("  *** %d/%d TESTS FAILED — FIX FAILURES ***\n",
               ktest_stats.failed, ktest_stats.total);
  else
    kio_printf("  All %d tests passed. Booting kernel.\n", ktest_stats.total);
}

static int ktest_run_category(int category, const char *label) {
  ktest_init();

#ifdef CONFIG_TESTS_SHOW_VERBOSE
  kio_printf("DICRON %s Test Harness\n", label);
  kio_printf("========================\n");
#else
  (void)label;
#endif

  for (const struct ktest_entry *e = __ktest_start; e < __ktest_end; e++) {
    if (e->category != category)
      continue;
#ifdef CONFIG_TESTS_SHOW_VERBOSE
    kio_printf("\n--- %s ---\n", e->name);
#endif
    int before_fail = ktest_stats.failed;
    e->fn();
    int suite_fails = ktest_stats.failed - before_fail;
    if (suite_fails > 0)
      kio_printf("  [SUITE FAIL] %s: %d assertion(s) failed\n", e->name,
                 suite_fails);
  }

  ktest_summary();
  return ktest_stats.failed;
}

int ktest_run_all(void) { return ktest_run_category(KTEST_CAT_BOOT, "Boot"); }

int ktest_run_post(void) {
  kio_printf("\n--- post-boot tests (live scheduler) ---\n");
  return ktest_run_category(KTEST_CAT_POST, "Post-Boot");
}
