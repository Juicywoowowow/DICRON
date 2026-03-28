/*
 * test_zram_compress.c — LZ4 compress/decompress roundtrip; compressible vs random
 */

#include "ktest.h"
#include "mm/zram.h"
#include "mm/lz4.h"
#include "mm/pmm.h"
#include "lib/string.h"
#include <dicron/mem.h>

KTEST_REGISTER(test_zram_compress, "ZRAM: compress/decompress roundtrip", KTEST_CAT_BOOT)
static void test_zram_compress(void)
{
	KTEST_BEGIN("ZRAM: compress/decompress roundtrip");

	zram_init();

	/* Test 1: highly compressible page (all zeros) */
	uint8_t page[4096];
	memset(page, 0, sizeof(page));

	int slot = zram_compress_page(page);
	KTEST_GE(slot, 0, "zero page compressed into ZRAM slot");
	if (slot < 0)
		return;

	uint8_t restored[4096];
	memset(restored, 0xFF, sizeof(restored));
	int ret = zram_decompress_page(slot, restored);
	KTEST_EQ(ret, 0, "zero page decompressed OK");
	KTEST_TRUE(memcmp(page, restored, 4096) == 0, "zero page roundtrip integrity");
	zram_free_slot(slot);

	/* Test 2: page with a known pattern */
	for (int i = 0; i < 4096; i++)
		page[i] = (uint8_t)(i & 0xFF);

	slot = zram_compress_page(page);
	KTEST_GE(slot, 0, "pattern page compressed into ZRAM slot");
	if (slot < 0)
		return;

	memset(restored, 0, sizeof(restored));
	ret = zram_decompress_page(slot, restored);
	KTEST_EQ(ret, 0, "pattern page decompressed OK");
	KTEST_TRUE(memcmp(page, restored, 4096) == 0, "pattern page roundtrip integrity");
	zram_free_slot(slot);

	/* Test 3: raw LZ4 compress/decompress of a compressible buffer */
	uint8_t src[4096];
	memset(src, 0xAB, sizeof(src));

	size_t worst = lz4_worst_compress(sizeof(src));
	void *cbuf = kmalloc(worst);
	void *wrkmem = kmalloc(LZ4_WORKMEM_SIZE);
	KTEST_NOT_NULL(cbuf, "LZ4 compress buffer allocated");
	KTEST_NOT_NULL(wrkmem, "LZ4 work memory allocated");

	if (cbuf && wrkmem) {
		size_t clen = worst;
		ret = lz4_compress(src, sizeof(src), cbuf, &clen, wrkmem);
		KTEST_EQ(ret, 0, "LZ4 compress succeeded");
		KTEST_TRUE(clen < sizeof(src), "compressed smaller than original");

		uint8_t dbuf[4096];
		size_t dlen = sizeof(dbuf);
		ret = lz4_decompress(cbuf, clen, dbuf, &dlen);
		KTEST_EQ(ret, 0, "LZ4 decompress succeeded");
		KTEST_EQ((long long)dlen, 4096, "decompressed to full page size");
		KTEST_TRUE(memcmp(src, dbuf, 4096) == 0, "LZ4 roundtrip integrity");
	}

	kfree(wrkmem);
	kfree(cbuf);
}
