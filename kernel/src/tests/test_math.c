#include "ktest.h"
#include "lib/math.h"

KTEST_REGISTER(ktest_math, "math operations", KTEST_CAT_BOOT)
static void ktest_math(void)
{
	KTEST_BEGIN("integer math library");

	KTEST_EQ(math_align_up(100, 32), 128, "align_up 100->128");
	KTEST_EQ(math_align_up(128, 32), 128, "align_up already aligned");
	KTEST_EQ(math_align_up(0, 4096), 0, "align_up zero");
	KTEST_EQ(math_align_down(100, 32), 96, "align_down 100->96");
	KTEST_EQ(math_align_down(128, 32), 128, "align_down already aligned");
	KTEST_EQ(math_page_align_up(4097), 8192, "page_align_up");
	KTEST_EQ(math_page_align_down(8191), 4096, "page_align_down");
	KTEST_EQ(math_div_ceil(10, 3), 4, "div_ceil 10/3");
	KTEST_EQ(math_div_ceil(9, 3), 3, "div_ceil exact");
	KTEST_EQ(math_div_ceil(1, 1), 1, "div_ceil 1/1");
	KTEST_EQ(math_div_ceil(0, 5), 0, "div_ceil 0/n");
	KTEST_EQ(math_log2_up(1), 0, "log2_up(1)");
	KTEST_EQ(math_log2_up(2), 1, "log2_up(2)");
	KTEST_EQ(math_log2_up(1024), 10, "log2_up(1024)");
	KTEST_EQ(math_log2_up(1025), 11, "log2_up(1025)");
	KTEST_EQ(math_max(5, 3), 5, "max(5,3)");
	KTEST_EQ(math_min(5, 3), 3, "min(5,3)");
	KTEST_EQ(math_abs(-42), 42, "abs(-42)");
	KTEST_EQ(math_abs(42), 42, "abs(42)");
	KTEST_EQ(math_max_u(0, 1), 1, "max_u(0,1)");
	KTEST_EQ(math_min_u(0, 1), 0, "min_u(0,1)");
}
