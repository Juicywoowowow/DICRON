/*
 * lz4.c — Minimal LZ4 block compress + decompress.
 *
 * Implements the LZ4 block format (no framing).
 * Each sequence is: [token] [literal_len_ext*] [literals] [offset]
 * [match_len_ext*] token: high nibble = literal length, low nibble = match
 * length - 4 If either nibble is 15, additional bytes follow (each 255 adds
 * 255). offset: 2 bytes little-endian back-reference distance.
 *
 *
 */

#include "lz4.h"
#include "lib/string.h"
#include <dicron/errno.h>

#define MINMATCH 4
#define LASTLITERALS 5
#define MFLIMIT 12

static inline uint32_t lz4_load32(const uint8_t *p) {
  uint32_t v;
  memcpy(&v, p, 4);
  return v;
}

static inline uint32_t lz4_hash(uint32_t v) {
  return (v * 2654435761U) >> (32 - LZ4_HASH_LOG);
}

/* ── compress ── */

int lz4_compress(const void *src, size_t src_len, void *dst, size_t *dst_len,
                 void *wrkmem) {
  if (!src || !dst || !dst_len || !wrkmem)
    return -EINVAL;

  const uint8_t *in = (const uint8_t *)src;
  uint8_t *out = (uint8_t *)dst;
  const uint8_t *in_end = in + src_len;
  const uint8_t *ip = in;
  uint8_t *op = out;
  const uint8_t *anchor = ip;
  const uint8_t *match_limit = in_end - LASTLITERALS;
  const uint8_t *mflimit = in_end - MFLIMIT;

  uint16_t *htab = (uint16_t *)wrkmem;
  memset(htab, 0, LZ4_HASH_SIZE * sizeof(uint16_t));

  if (src_len < MFLIMIT)
    goto emit_last_literals;

  ip++;

  for (;;) {
    const uint8_t *ref;
    size_t off;

    /* find a match */
    for (;;) {
      if (ip > mflimit)
        goto emit_last_literals;

      uint32_t h = lz4_hash(lz4_load32(ip));
      ref = in + htab[h];
      htab[h] = (uint16_t)(ip - in);

      off = (size_t)(ip - ref);
      if (off > 0 && off < 65536 && lz4_load32(ref) == lz4_load32(ip))
        break;
      ip++;
    }

    /* emit literals since anchor */
    {
      size_t lit_len = (size_t)(ip - anchor);
      uint8_t *token = op++;

      /* encode literal length */
      if (lit_len >= 15) {
        *token = 0xF0;
        size_t remain = lit_len - 15;
        while (remain >= 255) {
          *op++ = 255;
          remain -= 255;
        }
        *op++ = (uint8_t)remain;
      } else {
        *token = (uint8_t)(lit_len << 4);
      }

      /* copy literals */
      memcpy(op, anchor, lit_len);
      op += lit_len;

      /* extend the match */
      size_t match_len = MINMATCH;
      {
        const uint8_t *mp = ip + MINMATCH;
        const uint8_t *mr = ref + MINMATCH;
        while (mp < match_limit && *mp == *mr) {
          mp++;
          mr++;
        }
        match_len = (size_t)(mp - ip);
      }

      /* emit offset (little-endian) */
      *op++ = (uint8_t)(off & 0xFF);
      *op++ = (uint8_t)(off >> 8);

      /* encode match length (minus MINMATCH) in token low nibble */
      size_t ml_code = match_len - MINMATCH;
      if (ml_code >= 15) {
        *token = (uint8_t)(*token | 0x0F);
        size_t remain = ml_code - 15;
        while (remain >= 255) {
          *op++ = 255;
          remain -= 255;
        }
        *op++ = (uint8_t)remain;
      } else {
        *token = (uint8_t)(*token | ml_code);
      }

      ip += match_len;
      anchor = ip;
    }
  }

emit_last_literals: {
  size_t lit_len = (size_t)(in_end - anchor);
  uint8_t *token = op++;

  if (lit_len >= 15) {
    *token = 0xF0;
    size_t remain = lit_len - 15;
    while (remain >= 255) {
      *op++ = 255;
      remain -= 255;
    }
    *op++ = (uint8_t)remain;
  } else {
    *token = (uint8_t)(lit_len << 4);
  }

  memcpy(op, anchor, lit_len);
  op += lit_len;
}

  *dst_len = (size_t)(op - out);
  return 0;
}

/* ── decompress ── */

int lz4_decompress(const void *src, size_t src_len, void *dst,
                   size_t *dst_len) {
  if (!src || !dst || !dst_len)
    return -EINVAL;

  const uint8_t *ip = (const uint8_t *)src;
  const uint8_t *ip_end = ip + src_len;
  uint8_t *op = (uint8_t *)dst;
  uint8_t *op_end = op + *dst_len;

  for (;;) {
    if (ip >= ip_end)
      break;

    uint8_t token = *ip++;

    /* ── literals ── */
    size_t lit_len = (size_t)(token >> 4);
    if (lit_len == 15) {
      for (;;) {
        if (ip >= ip_end)
          return -ERANGE;
        uint8_t s = *ip++;
        lit_len += s;
        if (s < 255)
          break;
      }
    }

    if (ip + lit_len > ip_end || op + lit_len > op_end)
      return -ERANGE;
    memcpy(op, ip, lit_len);
    ip += lit_len;
    op += lit_len;

    /* last sequence has no match — check if we consumed all input */
    if (ip >= ip_end)
      break;

    /* ── match offset ── */
    if (ip + 2 > ip_end)
      return -ERANGE;
    size_t off = (size_t)ip[0] | ((size_t)ip[1] << 8);
    ip += 2;

    if (off == 0)
      return -EINVAL;
    uint8_t *ref = op - off;
    if (ref < (uint8_t *)dst)
      return -EINVAL;

    /* ── match length ── */
    size_t match_len = (size_t)(token & 0x0F) + MINMATCH;
    if ((token & 0x0F) == 15) {
      for (;;) {
        if (ip >= ip_end)
          return -ERANGE;
        uint8_t s = *ip++;
        match_len += s;
        if (s < 255)
          break;
      }
    }

    if (op + match_len > op_end)
      return -ERANGE;

    /* byte-by-byte copy for overlapping matches */
    for (size_t i = 0; i < match_len; i++)
      op[i] = ref[i];
    op += match_len;
  }

  *dst_len = (size_t)(op - (uint8_t *)dst);
  return 0;
}
