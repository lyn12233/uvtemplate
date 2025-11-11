#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"

typedef uint8_t u8;

static void buff_dump(const void *data, uint32_t len);
extern const u8 mod255[64];

void int_add(u8 out[64], const u8 a[64], const u8 b[64]) {
  uint16_t c = 0;
  for (int i = 0; i < 64; ++i) {
    c = (uint16_t)a[i] + b[i] + c;
    out[i] = c & 0xFF;
    c >>= 8;
  }
  // int_add_carry = (uint8_t)c;
}
void int_sub(u8 out[64], const u8 a[64], const u8 b[64]) {
  uint16_t c = 0;
  for (int i = 0; i < 64; ++i) {
    c = (uint16_t)a[i] - b[i] - c;
    out[i] = c & 0xFF;
    c = (c >> 8) & 1u; /* borrow is bit 0 of MSB extension */
  }
  // int_sub_borrow = (uint8_t)c;
}

void int_mul(u8 out[64], const u8 a[32], const u8 b[32]) {
  /* zero the full 512-bit result */
  u8 tmp[64];
  memset(tmp, 0, 64);

  /* classic O(n²) school-book multiplication */
  for (unsigned i = 0; i < 32; ++i) {
    uint16_t carry = 0;

    for (unsigned j = 0; j < 32; ++j) {
      /* 16-bit multiply + previous digit + carry */
      uint16_t prod = (uint16_t)a[i] * (uint16_t)b[j] + tmp[i + j] + carry;

      tmp[i + j] = (u8)prod; /* low byte  */
      carry = prod >> 8;     /* high byte */
    }
    tmp[i + 32] += (u8)carry; /* final carry may cascade */
  }

  /* copy result to caller buffer */
  memcpy(out, tmp, 64);
}

void int_shl(u8 res[64], const u8 m[64], int shift) {
  // printf("before shift-l: %d bits\n", shift);
  // buff_dump(m, 64);

  memset(res, 0, 64);
  if (shift <= 0) {
    if (!shift)
      memcpy(res, m, 64);
    return;
  }
  if (shift >= 512)
    return;

  int byteOff = shift >> 3;
  int bitOff = shift & 7;

  for (int i = 63; i >= 0; --i) {
    int src = i;
    int dst = i + byteOff;
    if (dst >= 64)
      continue;

    /* 16-bit window: high-byte from next-higher index */
    uint16_t w = m[src];
    if (bitOff && src < 63)
      w |= (uint16_t)m[src + 1] << 8;

    w <<= bitOff;

    res[dst] = w & 0xFF; /* low byte */
    if (dst + 1 < 64)
      res[dst + 1] = w >> 8; /* high byte */
  }

  // puts("after shift-l:");
  // buff_dump(res, 64);
  // exit(-1);
}

void int_shr_1(u8 res[64], const u8 m[64]) {
  // printf("before shift-r: 1 bits\n");
  // buff_dump(m, 64);
  int carry = 0;
  for (int i = 63; i >= 0; --i) {
    int newC = m[i] & 1u;
    res[i] = (m[i] >> 1) | (carry << 7);
    carry = newC;
  }
  // puts("after shift-l:");
  // buff_dump(res, 64);
  // exit(-1);
}

void int_mod(u8 res[64], const u8 m[64], const u8 mod[64]) {
  u8 tmp[64];
  memcpy(tmp, m, 64); /* working copy */

  // puts("mod:");
  // buff_dump(mod, 64);

  // find first 1
  int modBits = 511;
  while (modBits >= 0 && !(mod[modBits >> 3] & (1u << (modBits & 7u)))) {
    --modBits;
  }

  if (modBits < 0) { /* mod == 0 → return 0 */
    memset(res, 0, 64);
    return;
  }

  u8 aligned[64];
  int shift = 511 - modBits;
  int_shl(aligned, mod, shift);

  /* shift-subtract loop */
  for (; shift >= 0; --shift) {
    int ge = 1;
    for (int i = 63; i >= 0; --i) {
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
      int borrow = 0;
      for (int i = 0; i < 64; ++i) {
        int t = (int)tmp[i] - (int)aligned[i] - borrow;
        tmp[i] = t;
        borrow = (t >> 8) & 1;
      }
    }
    int_shr_1(aligned, aligned);
  }
  memcpy(res, tmp, 64);
}

void int_sub_mod(u8 out[64], const u8 a[64], const u8 b[64]) {
  u8 ta[64], tb[64], tmp[64];
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

void int_inv(u8 out[64], const u8 m[64]) {
  /* constant-time exponentiation: exponent = p-2 = 2^255-21 */
  static const u8 expo[64] = {
      0xEB, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F,
  };

  u8 base[64], acc[64], sq[64];

  /* base = m % p , acc = 1 */
  int_mod(base, m, mod255);
  memset(acc, 0, 64);
  acc[0] = 1;

  /* square-and-multiply MSB→LSB */
  for (int i = 255; i >= 0; --i) {
    /* sq = acc^2 */
    int_mul(sq, acc, acc);
    int_mod(acc, sq, mod255);

    int byte = i >> 3, bit = i & 7;
    if (expo[byte] & (1u << bit)) {
      /* acc *= base */
      int_mul(sq, acc, base);
      int_mod(acc, sq, mod255);
    }
  }
  memcpy(out, acc, 64);
}

static void buff_dump(const void *data, uint32_t len) {
  const uint8_t *p = data;
  for (size_t i = 0; i < len; i += 32) {
    // printf("%.4zu:", i);
    for (size_t j = i; j < i + 32; j++) {
      if (j < len)
        printf("%02x", p[j]);
      else {
      }
      // printf("\n");
    }
    printf("\n");
  }
}
const u8 mod255[64] = {
    0xED, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F,
    /* high 32 bytes = 0 */
};