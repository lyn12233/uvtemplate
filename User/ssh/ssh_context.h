#pragma once

#include "crypto/chacha.h"
#include "types/vo.h"

#include <stdint.h>

typedef struct {
  vstr_t v_c, v_s; // version headers
  vstr_t i_c, i_s; // kexinit raw
  vstr_t k_s;      // server host key
  union {
    vstr_t a; // client ephemeral priv key
    vstr_t b; // server ephemeral priv key
  };
  vstr_t q_c, q_s; // ephemeral public key
  vstr_t k;        // shared secret K
  vstr_t h;        // hash H

  int first_kex;

  vstr_t sid;     // session id, first H
  vstr_t iv_c2s;  // 'A' 64 bytes
  vstr_t iv_s2c;  // 'B' 64 bytes
  vstr_t enc_c2s; // 'C' 64 bytes
  vstr_t enc_s2c; // 'D' 64 bytes
  vstr_t mac_c2s; // 'E' 64 bytes
  vstr_t mac_s2c; // 'F' 64 bytes

  struct {
    uint32_t seq_number;
    struct chacha_ctx ctx_main, ctx_hdr;
  } c2s, s2c;
  //...
} ssh_context;

// ssh context contains materials to construct hash for echd reply. definition
// see RFC 5656 chapter 4; implementation see input_kex_gen_init() (whole phases
// of ecdh_init+reply) (in kexgen.c:276), kex_c25519_enc() (encode the shared
// secret) (in kex.c), kex_gen_hash() (generate hash by materials) (in
// kexgen.c:48); it is indicated that elements of hash material is a
// packet-style string/mpint and payload includes ssh_opcode part.
// sshbuf_put_xxx is to concatenate a bare string, while sshbuf_put_string_x is
// used to append both length and buffer.

ssh_context *ssh_context_create();
void ssh_context_init(ssh_context *ctx);
void ssh_context_delete(ssh_context *ctx);
void ssh_context_clear(ssh_context *ctx);