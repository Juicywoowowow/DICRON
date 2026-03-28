#ifndef _DICRON_MM_LZ4_H
#define _DICRON_MM_LZ4_H

#include <stddef.h>
#include <stdint.h>

/*
 * Minimal LZ4 block compress / decompress for kernel page compression.
 *
 * lz4_compress():
 *   src     — input buffer
 *   src_len — input length (typically PAGE_SIZE)
 *   dst     — output buffer (must be at least lz4_worst_compress(src_len))
 *   dst_len — [in/out] on entry: dst capacity; on return: compressed size
 *   wrkmem  — scratch buffer, LZ4_WORKMEM_SIZE bytes
 *   Returns 0 on success, negative errno on failure.
 *
 * lz4_decompress():
 *   src     — compressed buffer
 *   src_len — compressed length
 *   dst     — output buffer (must be >= original uncompressed size)
 *   dst_len — [in/out] on entry: dst capacity; on return: decompressed size
 *   Returns 0 on success, negative errno on failure.
 */

#define LZ4_HASH_LOG		12
#define LZ4_HASH_SIZE		(1U << LZ4_HASH_LOG)
#define LZ4_WORKMEM_SIZE	(LZ4_HASH_SIZE * sizeof(uint16_t))

/* worst-case compressed output size for n input bytes */
static inline size_t lz4_worst_compress(size_t n)
{
	return n + (n / 255) + 16;
}

int lz4_compress(const void *src, size_t src_len,
		 void *dst, size_t *dst_len,
		 void *wrkmem);

int lz4_decompress(const void *src, size_t src_len,
		   void *dst, size_t *dst_len);

#endif
