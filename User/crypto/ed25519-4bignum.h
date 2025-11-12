#include "types/vo.h"

#include "crypto_api.h"
#ifdef USE_CRYPTO_V2

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"

typedef uint32_t u32;
typedef uint8_t u8;

extern const u32 mod255[16];

void int_add(u32 out[16], const u32 a[16], const u32 b[16]) {
  uint64_t c = 0;
  for (int i = 0; i < 16; ++i) {
    c = (uint64_t)a[i] + (uint64_t)b[i] + c;
    out[i] = (u32)(c & 0xFFFFFFFFu);
    c >>= 32;
  }
  // int_add_carry = (uint32_t)c;
}
void int_sub(u32 out[16], const u32 a[16], const u32 b[16]) {
  uint64_t c = 0;
  for (int i = 0; i < 16; ++i) {
    c = (uint64_t)a[i] - (uint64_t)b[i] - c;
    out[i] = (u32)(c & 0xFFFFFFFFu);
    c = (c >> 32) & 1u; /* borrow is bit 0 of MSB extension */
  }
  // int_sub_borrow = (uint32_t)c;
}

void int_mul(u32 out[16], const u32 a[8], const u32 b[8]) {
  /* zero the full 512-bit result (16 limbs) */
  u32 tmp[16];
  memset(tmp, 0, sizeof tmp);

  /* classic O(n²) school-book multiplication */
  for (unsigned i = 0; i < 8; ++i) {
    uint64_t carry = 0;

    for (unsigned j = 0; j < 8; ++j) {
      /* 64-bit multiply + previous digit + carry */
      uint64_t prod =
          (uint64_t)a[i] * (uint64_t)b[j] + (uint64_t)tmp[i + j] + carry;

      tmp[i + j] = (u32)prod; /* low 32 bits  */
      carry = prod >> 32;     /* high 32 bits */
    }
    tmp[i + 8] += (u32)carry; /* final carry may cascade */
  }

  /* copy result to caller buffer */
  memcpy(out, tmp, sizeof tmp);
}

void int_shl(u32 res[16], const u32 m[16], int shift) {
  // printf("before shift-l: %d bits\n", shift);
  // buff_dump(m, 64);

  memset(res, 0, 16 * sizeof(u32));
  if (shift <= 0) {
    if (!shift)
      memcpy(res, m, 16 * sizeof(u32));
    return;
  }
  if (shift >= 512)
    return;

  int wordOff = shift >> 5; /* shift / 32 */
  int bitOff = shift & 31;

  for (int i = 15; i >= 0; --i) {
    int src = i;
    int dst = i + wordOff;
    if (dst >= 16)
      continue;

    /* 64-bit window: high-word from next-higher index */
    uint64_t w = m[src];
    if (bitOff && src < 15)
      w |= (uint64_t)m[src + 1] << 32;

    w <<= bitOff;

    res[dst] = (u32)(w & 0xFFFFFFFFu); /* low 32 bits */
    if (dst + 1 < 16)
      res[dst + 1] = (u32)(w >> 32); /* high 32 bits */
  }

  // puts("after shift-l:");
  // buff_dump(res, 64);
  // exit(-1);
}

void int_shr_1(u32 res[16], const u32 m[16]) {
  // printf("before shift-r: 1 bits\n");
  // buff_dump(m, 64);
  int carry = 0;
  for (int i = 15; i >= 0; --i) {
    int newC = m[i] & 1u;
    res[i] = (m[i] >> 1) | ((u32)carry << 31);
    carry = newC;
  }
  // puts("after shift-l:");
  // buff_dump(res, 64);
  // exit(-1);
}

void int_mod(u32 res[16], const u32 m[16], const u32 mod[16]) {
  u32 tmp[16];
  memcpy(tmp, m, 16 * sizeof(u32)); /* working copy */

  // puts("mod:");
  // buff_dump(mod, 64);

  /* find first 1 bit in mod (0..511) */
  int modBits = 511;
  while (modBits >= 0) {
    int w = modBits >> 5;
    int b = modBits & 31;
    if (mod[w] & (1u << b))
      break;
    --modBits;
  }

  if (modBits < 0) { /* mod == 0 → return 0 */
    memset(res, 0, 16 * sizeof(u32));
    return;
  }

  u32 aligned[16];
  int shift = 511 - modBits;
  int_shl(aligned, mod, shift);

  /* shift-subtract loop */
  for (; shift >= 0; --shift) {
    int ge = 1;
    for (int i = 15; i >= 0; --i) {
      if (tmp[i] > aligned[i]) {
        ge = 1;
        break;
      }
      if (tmp[i] < aligned[i]) {
        ge = 0;
        break;
      }
    }
    if (ge) {
      uint64_t borrow = 0;
      for (int i = 0; i < 16; ++i) {
        int64_t t = (int64_t)tmp[i] - (int64_t)aligned[i] - (int64_t)borrow;
        tmp[i] = (u32)t;
        borrow = (t >> 32) & 1;
      }
    }
    int_shr_1(aligned, aligned);
  }
  memcpy(res, tmp, 16 * sizeof(u32));
}

void int_sub_mod(u32 out[16], const u32 a[16], const u32 b[16]) {
  u32 ta[16], tb[16], tmp[16];
  /* ta = a % mod255 */
  int_mod(ta, a, mod255);
  /* tb = b % mod255 */
  int_mod(tb, b, mod255);
  int_add(tmp, ta, mod255);
  int_sub(tmp, tmp, tb);
  int_mod(out, tmp, mod255);

  // puts("tmp,out:");
  // buff_dump(tmp, 64);
  // buff_dump(out, 64);
}

void int_inv(u32 out[16], const u32 m[16]) {
  /* constant-time exponentiation: exponent = p-2 = 2^255-21 */
  static const u32 expo[16] = {
      0xFFFFFFEBu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu,
      0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0x7FFFFFFFu,
      0u,          0u,          0u,          0u,
      0u,          0u,          0u,          0u,
  };

  u32 base[16], acc[16], sq[16];

  /* base = m % p , acc = 1 */
  int_mod(base, m, mod255);
  memset(acc, 0, 16 * sizeof(u32));
  acc[0] = 1;

  /* square-and-multiply MSB→LSB */
  for (int i = 255; i >= 0; --i) {
    /* sq = acc^2 */
    int_mul(sq, acc, acc);
    int_mod(acc, sq, mod255);

    /* test expo bit i: expo is stored as u32 words (little-endian bytes packed)
     */
    int byte = i >> 3;           /* which byte */
    int word = byte >> 2;        /* which u32 word */
    int byte_in_word = byte & 3; /* byte index inside word */
    int bit = i & 7;
    int bit_index = byte_in_word * 8 + bit; /* 0..31 inside the u32 word */
    if (expo[word] & (1u << bit_index)) {
      /* acc *= base */
      int_mul(sq, acc, base);
      int_mod(acc, sq, mod255);
    }
  }
  memcpy(out, acc, 16 * sizeof(u32));
}

const u32 mod255[16] = {
    0xFFFFFFEDu,
    0xFFFFFFFFu,
    0xFFFFFFFFu,
    0xFFFFFFFFu,
    0xFFFFFFFFu,
    0xFFFFFFFFu,
    0xFFFFFFFFu,
    0x7FFFFFFFu,
    /* high 8 words = 0 */
    0u,
    0u,
    0u,
    0u,
    0u,
    0u,
    0u,
    0u,
};

#endif