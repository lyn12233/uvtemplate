/* $OpenBSD: hash.c,v 1.6 2019/11/29 00:11:21 djm Exp $ */
/*
 * Public domain. Author: Christian Weisgerber <naddy@openbsd.org>
 * API compatible reimplementation of function from nacl
 */

#include "crypto_api.h"

#include <stdarg.h>

#include "sha2.h"

#include "log.h"

int crypto_hash_sha512(unsigned char *out, const unsigned char *in,
                       unsigned long long inlen) {

  SHA2_CTX ctx;

  SHA512Init(&ctx);
  SHA512Update(&ctx, in, inlen);
  SHA512Final(out, &ctx);
  return 0;
}

int crypto_hash_sha256(unsigned char *out, const unsigned char *in,
                       unsigned long long inlen) {
  SHA2_CTX ctx;
  SHA256Init(&ctx);
  SHA256Update(&ctx, in, inlen);
  SHA256Final(out, &ctx);
  return 0;
}