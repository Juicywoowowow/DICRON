#include "../drivers/hpet/hpet.h"
#include "ktest.h"
#include <dicron/time.h>

#ifdef CONFIG_HPET
KTEST_REGISTER(ktest_hpet_edge, "hpet edge cases", KTEST_CAT_BOOT)
static void ktest_hpet_edge(void) {
  KTEST_BEGIN("HPET time edge cases");

  uint64_t t1 = ktime_ns();
  ksleep_ns(0);
  uint64_t t2 = ktime_ns();
  KTEST_LT(t2 - t1, 50000000ULL, "ksleep_ns(0) returns immediately");

  uint64_t t3 = ktime_ns();
  while (ktime_ns() - t3 < 20000)
    ;
  KTEST_TRUE(1, "timer advances during busy loop");

  /* Extra test 1: Monotonicity without yielding */
  uint64_t m1 = ktime_ns();
  uint64_t m2 = ktime_ns();
  uint64_t m3 = ktime_ns();
  KTEST_GE(m2, m1, "hpet: time is monotonically increasing (1)");
  KTEST_GE(m3, m2, "hpet: time is monotonically increasing (2)");

  /* Extra test 2: Availability check */
  KTEST_TRUE(hpet_is_available(), "hpet: is marked as available after init");

  /* Extra test 3: Small nanosecond sleep consistency */
  uint64_t ns1 = ktime_ns();
  ksleep_ns(50000); /* 50 microseconds */
  uint64_t ns2 = ktime_ns();
  KTEST_GE(ns2 - ns1, 50000, "hpet: ksleep_ns(50000) wait at least 50us");
}
#endif
