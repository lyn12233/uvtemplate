#include "ed25519-4bignum.c"

#include <stdint.h>

#include "log.h"

typedef struct {
  u8 x[64];
  u8 y[64];
  u8 z[64];
  u8 t[64];
} ge;

void extended_homogeneous_to_affine(u8 x[64], u8 y[64], const ge *Q) {
  u8 zinv[64];
  int_inv(zinv, Q->z);
  int_mul(x, Q->x, zinv);
  int_mod(x, x, mod255);
  int_mul(y, Q->y, zinv);
  int_mod(y, y, mod255);
}

void affine_to_extended_homogeneous(ge *Q, const u8 x[64], const u8 y[64]) {
  memcpy(Q->x, x, 64);
  memcpy(Q->y, y, 64);
  memset(Q->z, 0, 64);
  Q->z[0] = 1;
  int_mul(Q->t, x, y);
  int_mod(Q->t, Q->t, mod255);
}

void add_ge(ge *out, const ge *q1, const ge *q2) {

  int log = q1->x[0] == 0xb0;
  if (log || 1) {
    // puts("ge add:");
    // buff_dump(q1, 64 * 4);
    // buff_dump(q2, 64 * 4);
  }

  u8 A[64], B[64], C[64], D[64], E[64], F[64], G[64], H[64];
  u8 two[64] = {2}, twod[64];

  /* ---- pre-compute 2*d mod p (d = -121665/121666) -----------------
     Ed25519 uses d = -121665 * inv(121666) mod p.
     Here we assume twod[] is already the constant 2*d mod p.
     If you store it elsewhere, just copy it in. -------------------- */
  /* 2*d = 0x42A0E5AA4F18B930E1E6E5E6D14B5C7A2D2A8C6F15D7B9A1F1E6E5E6...
   * (64-byte LE) */
  static const u8 twod_const[64] = {
      0x59, 0xF1, 0xB2, 0x26, 0x94, 0x9B, 0xD6, 0xEB, 0x56, 0xB1, 0x83, 0x82,
      0x9A, 0x14, 0xE0, 0x00, 0x30, 0xD1, 0xF3, 0xEE, 0xF2, 0x80, 0x8E, 0x19,
      0xE7, 0xFC, 0xDF, 0x56, 0xDC, 0xD9, 0x06, 0x24,
      /* high 32 bytes = 0 */
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  memcpy(twod, twod_const, 64);

  /* A = (Y1 - X1)*(Y2 - X2) mod p */
  int_sub_mod(A, q1->y, q1->x);
  int_sub_mod(H, q2->y, q2->x); /* H used as temp */
  int_mul(A, A, H);
  int_mod(A, A, mod255);

  /* B = (Y1 + X1)*(Y2 + X2) mod p */
  int_add(H, q1->y, q1->x);
  int_add(E, q2->y, q2->x); /* E used as temp */
  int_mul(B, H, E);
  int_mod(B, B, mod255);

  /* C = (T1 * 2 * d * T2) mod p  →  C = T1 * (2*d) * T2  mod p */
  int_mul(C, q1->t, twod);
  int_mod(C, C, mod255);
  int_mul(C, C, q2->t);
  int_mod(C, C, mod255);

  /* D = (Z1 * 2 * Z2) mod p  →  D = 2 * Z1 * Z2  mod p */
  int_mul(D, two, q1->z);
  int_mod(D, D, mod255);
  int_mul(D, D, q2->z);
  int_mod(D, D, mod255);

  /* E = (B - A) mod p */
  int_sub_mod(E, B, A);

  /* F = (D - C) mod p */
  int_sub_mod(F, D, C);

  /* G = (D + C) mod p */
  int_add(G, D, C);
  int_mod(G, G, mod255);

  /* H = (B + A) mod p */
  int_add(H, B, A);
  int_mod(H, H, mod255);

  /* X3 = E * F mod p */
  int_mul(out->x, E, F);
  int_mod(out->x, out->x, mod255);

  /* Y3 = G * H mod p */
  int_mul(out->y, G, H);
  int_mod(out->y, out->y, mod255);

  /* T3 = E * H mod p */
  int_mul(out->t, E, H);
  int_mod(out->t, out->t, mod255);

  /* Z3 = F * G mod p */
  int_mul(out->z, F, G);
  int_mod(out->z, out->z, mod255);

  if (log || 1) {
    // puts("add result:");
    // buff_dump(out, 64 * 4);
    // puts("A:");
    // buff_dump(A, 64);
    // puts("B:");
    // buff_dump(B, 64);
    // puts("C:");
    // buff_dump(C, 64);
    // puts("D:");
    // buff_dump(D, 64);
    // puts("E:");
    // buff_dump(E, 64);
    // puts("F:");
    // buff_dump(F, 64);
    // puts("G:");
    // buff_dump(G, 64);
    // puts("H:");
    // buff_dump(H, 64);
  }
  if (log) {
    // exit(-1);
  }
}

void double_ge(ge *out, const ge *q) {
  // puts("ge double:");
  // buff_dump(q, 64 * 4);

  u8 A[64], B[64], C[64], H[64], E[64], F[64], G[64];
  u8 XY[64], two[64] = {2};

  /* A = X1^2 mod p */
  int_mul(A, q->x, q->x);
  int_mod(A, A, mod255);

  /* B = Y1^2 mod p */
  int_mul(B, q->y, q->y);
  int_mod(B, B, mod255);

  /* C = 2*Z1^2 mod p  →  C = 2*(Z1^2) mod p */
  int_mul(C, q->z, q->z);
  int_mod(C, C, mod255);
  int_mul(C, two, C);
  int_mod(C, C, mod255);

  /* H = A + B mod p */
  int_add(H, A, B);
  int_mod(H, H, mod255);

  /* E = H - (X1+Y1)^2 mod p  →  first (X1+Y1)^2 */
  int_add(XY, q->x, q->y);
  int_mul(XY, XY, XY);
  int_mod(XY, XY, mod255);
  int_sub_mod(E, H, XY); /* E = H - (X1+Y1)^2 */

  /* G = A - B mod p */
  int_sub_mod(G, A, B);

  /* F = C + G mod p */
  int_add(F, C, G);
  int_mod(F, F, mod255);

  /* X3 = E * F mod p */
  int_mul(out->x, E, F);
  int_mod(out->x, out->x, mod255);

  /* Y3 = G * H mod p */
  int_mul(out->y, G, H);
  int_mod(out->y, out->y, mod255);

  /* T3 = E * H mod p */
  int_mul(out->t, E, H);
  int_mod(out->t, out->t, mod255);

  /* Z3 = F * G mod p */
  int_mul(out->z, F, G);
  int_mod(out->z, out->z, mod255);

  // puts("A:");
  // buff_dump(A, 64);
  // puts("B:");
  // buff_dump(B, 64);
  // puts("C:");
  // buff_dump(C, 64);
  // puts("XY (X1+Y1)^2:");
  // buff_dump(XY, 64);
  // puts("E:");
  // buff_dump(E, 64);
  // puts("F:");
  // buff_dump(F, 64);
  // puts("G:");
  // buff_dump(G, 64);
  // puts("H:");
  // buff_dump(H, 64);
  // puts("double result:");
  // buff_dump(out, 64 * 4);
  // exit(-1);
}

void ge_mul(ge *q, const u8 s[32], const ge *p) {
  ge Q, P, tmp;
  u8 b;

  /* ----- start dump ----- */

  // printf("point_mul: ");
  // buff_dump(s, 32);

  // puts("");
  // buff_dump(p->x, 32);
  // puts(",");
  // buff_dump(p->y, 32);
  // puts(",");
  // buff_dump(p->z, 32);
  // puts(",");
  // buff_dump(p->t, 32);

  // printf("\n");
  /* ----- end dump ----- */

  /* Q = neutral element (0,1,1,0) */
  memset(Q.x, 0, 64);
  Q.x[0] = 0;
  memset(Q.y, 0, 64);
  Q.y[0] = 1;
  memset(Q.z, 0, 64);
  Q.z[0] = 1;
  memset(Q.t, 0, 64);
  Q.t[0] = 0;

  memcpy(&P, p, sizeof(ge)); /* P = input point */

  /* scan each bit of s (little-endian, 256 bits) */
  // for (int byte = 0; byte < 32; ++byte) {
  for (int byte = 31; byte >= 0; byte--) {
    b = s[byte];
    // for (int bit = 0; bit < 8; ++bit) {
    for (int bit = 7; bit >= 0; bit--) {
      // printf("step %u\n", 255 - (byte * 8 + bit));

      if (!((b >> bit) & 1)) { /* bit = 0 */
        // printf("0");
        add_ge(&tmp, &Q, &P); /* P = Q+P */
        double_ge(&Q, &Q);    /* Q = 2Q  */
        memcpy(&P, &tmp, sizeof(ge));

      } else { /* bit = 0 */

        // printf("1");
        add_ge(&tmp, &Q, &P); /* Q = Q+P */
        double_ge(&P, &P);    /* P = 2P  */
        memcpy(&Q, &tmp, sizeof(ge));
      }

      // log
      // puts("after iter:");
      // buff_dump(&P, 64 * 4);
      // buff_dump(&Q, 64 * 4);

      if (255 - (byte * 8 + bit) == 30) {
        // exit(-1);
      }
    }
  }
  memcpy(q, &Q, sizeof(ge));

  // puts("result:");
  // buff_dump(q, 64 * 4);
}

static const ge G_4 = {
    .x = {0x1a, 0xd5, 0x25, 0x8f, 0x60, 0x2d, 0x56, 0xc9, 0xb2, 0xa7, 0x25,
          0x95, 0x60, 0xc7, 0x2c, 0x69, 0x5c, 0xdc, 0xd6, 0xfd, 0x31, 0xe2,
          0xa4, 0xc0, 0xfe, 0x53, 0x6e, 0xcd, 0xd3, 0x36, 0x69, 0x21},
    .y = {0x58, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
          0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
          0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66},
    .z = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    .t = {0xa3, 0xdd, 0xb7, 0xa5, 0xb3, 0x8a, 0xde, 0x6d, 0xf5, 0x52, 0x51,
          0x77, 0x80, 0x9f, 0xf0, 0x20, 0x7d, 0xe3, 0xab, 0x64, 0x8e, 0x4e,
          0xea, 0x66, 0x65, 0x76, 0x8b, 0xd7, 0x0f, 0x5f, 0x87, 0x67}};

void raw_pub_from_raw_material(uint8_t *out, const uint8_t *mat) {
  ge Q;
  uint8_t x[64], y[64];

  // mat[0] &= 248;
  // mat[31] &= 127;
  // mat[31] |= 64;

  ge_mul(&Q, mat, &G_4);
  extended_homogeneous_to_affine(x, y, &Q);

  // puts("raw: middle result:");
  // buff_dump(&Q, sizeof(Q));

  memcpy(out, y, 32);
  out[31] |= (x[0] & 1) << 7;
}
void raw_pub_from_raw_material64(uint8_t *out, const uint8_t *mat) {
  static const u8 L64[64] = {
      /* low 32 bytes: L (little-endian) */
      0xed, 0xd3, 0xf5, 0x5c, 0x1a, 0x63, 0x12, 0x58, 0xd6, 0x9c, 0xf7, 0xa2,
      0xde, 0xf9, 0xde, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10,
      /* high 32 bytes: zeros */
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  u8 s64[64];

  int_mod(s64, mat, L64);
  raw_pub_from_raw_material(out, s64);
}