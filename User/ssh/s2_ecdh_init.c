
#include "s2_ecdh_init.h"
#include "crypto/crypto_api.h"
#include "esp/espsock.h"
#include "packet.h"
#include "packet_def.h"
#include "ssh/packet.h"
#include "types/vo.h"

#include "port_unistd.h"

#include <stdint.h>
#include <string.h>

#include "log.h"

static void ecdh_calc_secret(                                             //
    const vstr_t *q_c, const vstr_t *b, vstr_t *q_s, vstr_t *x, vstr_t *k //
); // post ecdh init

static void ecdh_calc_hash(ssh_context *ctx, vstr_t *h);            // pre reply
static void ecdh_sign_hash(ssh_context *ctx, vstr_t *h, vstr_t *s); // pre reply

static void ecdh_derive_keys(ssh_context *ctx); // post reply

static const char *blob_head = "ssh-ed25519";
static const uint32_t blob_head_len = 11;

int is_first_kex = 1;

void consume_ecdh_init(int sock, ssh_context *ctx, int *done) {
  vstr_t vbuff;
  vstr_init(&vbuff, 0);
  size_t payload_len;

  // recv
  int res = recv_packet(sock, &vbuff, &payload_len);
  if (res < 0) {
    puts("consume edchinit: recv failed");
    vstr_clear(&vbuff);
    *done = 0;
    return;
  }

  // decode
  printf("ecdh: parse ecdh init\r\n");
  vbuff_dump(&vbuff);
  vo_t *otmp = payload_decode(vbuff.buff + 1, payload_len, ecdhinit_types,
                              LEN_ECHDINIT_TYPES);

  vstr_clear(&vbuff);
  (void)vbuff;

  // repr
  printf("packet received length %d, payload:\n", (int)payload_len);
  vo_repr(otmp);

  // validate and store
  if (!(otmp->vlist.nb == 2 && otmp->vlist.data[1].type == vot_string &&
        otmp->vlist.data[1].vstr.len == 32)) {
    puts("consume ecdh init: invalid struct");
    vo_delete(otmp);
    *done = 0;
    return;
  }
  ctx->q_c.len = 0;
  vbuff_iadd(&ctx->q_c, otmp->vlist.data[1].vstr.data, 32);

  vo_delete(otmp);
  (void)otmp;

  // post-consume: generate a random b(server ehpemeral priv),
  // then generate Q_s, K
  puts("consume_ecdh_init: post consume");

  // generate b: 32 byte ephemeral server priv key
  vstr_reserve(&ctx->b, 32);
  ctx->b.len = 32;
  ctx->b.data[0] = 0xff;
  ctx->b.data[0] &= 248;
  ctx->b.data[31] &= 127;
  ctx->b.data[31] |= 64;

  // calc secret q_s = [b] q_c = [ab] Base
  ecdh_calc_secret(&ctx->q_c, &ctx->b, &ctx->q_s, NULL, &ctx->k);

  // k_s: server host key; set as blobbed xample_pub_key {string,string}
  ctx->k_s.len = 0;
  vbuff_iaddu32(&ctx->k_s, blob_head_len);
  vbuff_iadd(&ctx->k_s, blob_head, blob_head_len);
  vbuff_iaddu32(&ctx->k_s, sizeof(xample_pub_key));
  vbuff_iadd(&ctx->k_s, (void *)xample_pub_key, sizeof(xample_pub_key));

  *done = 1;
}

void send_ecdh_reply(int sock, ssh_context *ctx, int *done) {

  // preprocess: calc H, S

  vstr_t s;
  vstr_init(&s, 0);
  ctx->h.len = 0;

  // hash
  ecdh_calc_hash(ctx, &ctx->h);
  // sign to s
  ecdh_sign_hash(ctx, &ctx->h, &s);

  // send message<tmp,vbuff> {31,ks,qs,s}

  vstr_t tmp;
  vstr_init(&tmp, 0);
  // opcode
  vbuff_iaddc(&tmp, 31);

  // ks
  if (ctx->k_s.len != 4 + 11 + 4 + 32) {
    puts("send ecdh: invalid k_s length");
    vstr_clear(&tmp);
    *done = 0;
    return;
  }
  vbuff_iaddu32(&tmp, ctx->k_s.len); // is deblobbed ctx->k_s
  vbuff_iadd(&tmp, ctx->k_s.data, ctx->k_s.len);

  // qs
  if (ctx->q_s.len != 32) {
    puts("send ecdh: invalid q_s lenth");
    vstr_clear(&tmp);
    *done = 0;
    return;
  }
  vbuff_iaddu32(&tmp, ctx->q_s.len);
  vbuff_iadd(&tmp, ctx->q_s.data, ctx->q_s.len);

  // signature (a blob, see ssh_ed25519:186 (ssh_ed25519_store_sig called by
  // ssh_ed25519_sign))
  if (s.len != 64) {
    puts("send ecdh: invalid s length");
    vstr_clear(&tmp);
    *done = 0;
    return;
  }
  vbuff_iaddu32(&tmp, 4 + blob_head_len + 4 + s.len);
  vbuff_iaddu32(&tmp, blob_head_len);
  vbuff_iadd(&tmp, blob_head, blob_head_len);
  vbuff_iaddu32(&tmp, s.len);
  vbuff_iadd(&tmp, s.data, s.len);

  vstr_clear(&s);
  (void)s;

  // create packet
  vstr_t *packet = payload2packet(&tmp, 4);
  printf("ecdh reply package len: %d\r\n", packet->len);

  vstr_clear(&tmp);
  (void)tmp;

  uint32_t len_to_send = htonl(packet->len);
  const vstr_t vsend = (vstr_t){.len = 4, .buff = (void *)&len_to_send};
  sock_send(sock, &vsend, 4, 0);
  sock_send(sock, packet, packet->len, 0);

  vstr_delete(packet);
  (void)packet;

  // post-process: assign sid, calc key a-f
  if (ctx->first_kex) {
    puts("ecdh: create session_id and init seq_num once");
    ctx->first_kex = 0;

    // sid=h
    ctx->sid.len = 0;
    vbuff_iadd(&ctx->sid, ctx->h.data, ctx->h.len);

    // sequence number
    ctx->c2s.seq_number = 3;
    ctx->s2c.seq_number = 3;

    ecdh_derive_keys(ctx);

    // setup chacha c2s
    const uint8_t *mainkey = (uint8_t *)ctx->enc_c2s.data;
    const uint8_t *hdrkey = (uint8_t *)ctx->enc_c2s.data + 32;
    memset(&ctx->c2s.ctx_main, 0, sizeof(struct chacha_ctx));
    memset(&ctx->c2s.ctx_hdr, 0, sizeof(struct chacha_ctx));
    chacha_keysetup(&ctx->c2s.ctx_main, mainkey, 256);
    chacha_keysetup(&ctx->c2s.ctx_hdr, hdrkey, 256);

    // setup chacha s2c
    mainkey = (uint8_t *)ctx->enc_s2c.data;
    hdrkey = (uint8_t *)ctx->enc_s2c.data + 32;
    memset(&ctx->s2c.ctx_main, 0, sizeof(struct chacha_ctx));
    memset(&ctx->s2c.ctx_hdr, 0, sizeof(struct chacha_ctx));
    chacha_keysetup(&ctx->s2c.ctx_main, mainkey, 256);
    chacha_keysetup(&ctx->s2c.ctx_hdr, hdrkey, 256);
  } else {
    ecdh_derive_keys(ctx);
  } // if(first_kex)
}

void exchange_msg_newkey(int sock, int *done) {

  // send newkeys

  vstr_t tmp;
  vstr_init(&tmp, 0);

  vbuff_iaddc(&tmp, SSH_MSG_NEWKEYS);
  vstr_t *packet = payload2packet(&tmp, 4);

  // vstr_clear(&tmp);
  // (void)tmp;

  uint32_t len_to_send = htonl(packet->len);
  const vstr_t vsend = (vstr_t){.len = 4, .buff = (void *)&len_to_send};
  sock_send(sock, &vsend, 4, 0);
  sock_send(sock, packet, packet->len, 0);
  puts("sent newkey message");

  vstr_delete(packet);
  (void)packet;

  // consume newkeys

  uint32_t payload_len;
  tmp.len = 0;
  int res = recv_packet(sock, &tmp, &payload_len);
  if (res < 0) {
    puts("ecdh newkeys: recv failed");
    vstr_clear(&tmp);
    *done = 0;
    return;
  }
  puts("expecting newkey:");
  buff_dump(tmp.buff, tmp.len);
  if (tmp.len >= 1 && tmp.buff[1] == SSH_MSG_NEWKEYS) {
    puts("recv newkey");
    *done = 1;
  } else {
    printf("not newkey: %d\r\n", tmp.buff[1]);
    *done = 0;
  }

  vstr_clear(&tmp);
}

// local funcs

static void ecdh_calc_secret(                                             //
    const vstr_t *q_c, const vstr_t *b, vstr_t *q_s, vstr_t *x, vstr_t *k //
) {
  uint8_t shared[32];
  uint8_t qs[32];
  static const uint8_t basepoint[32] = {9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  // X = X25519(b, Q_c)
  crypto_scalarmult_curve25519(shared, b->buff, q_c->buff);
  puts("calc x");

  // Q_s = X25519(b, 9)
  crypto_scalarmult_curve25519(qs, b->buff, basepoint);
  puts("calc qs");

  // clear and assign results into vstr_t
  if (x) { // x nullable
    x->len = 0;
    vbuff_iadd(x, (void *)shared, 32);
  }
  q_s->len = 0;
  vbuff_iadd(q_s, (void *)qs, 32);
  puts("assign q_s");

  // calc K
  // see kexc25519_shared_key_ext
  k->len = 0;
  vbuff_iaddmp(k, (void *)shared, 32);
  puts("assign to k");
}

static void ecdh_calc_hash(ssh_context *ctx, vstr_t *h) {
  vstr_t tmp;
  vstr_init(&tmp, 0);

  // assertions
  assert(ctx->v_c.data[ctx->v_c.len - 1] != '\n');
  assert(ctx->k_s.len == 32 + 11 + 8);
  assert(ctx->q_c.len == 32);
  assert(ctx->q_s.len == 32);

  // vc
  vbuff_iaddu32(&tmp, ctx->v_c.len);
  vbuff_iadd(&tmp, ctx->v_c.data, ctx->v_c.len);
  // vs
  vbuff_iaddu32(&tmp, ctx->v_s.len);
  vbuff_iadd(&tmp, ctx->v_s.data, ctx->v_s.len);
  // ic
  vbuff_iaddu32(&tmp, ctx->i_c.len);
  vbuff_iadd(&tmp, ctx->i_c.data, ctx->i_c.len);
  // is
  vbuff_iaddu32(&tmp, ctx->i_s.len);
  vbuff_iadd(&tmp, ctx->i_s.data, ctx->i_s.len);
  // ks
  vbuff_iaddu32(&tmp, ctx->k_s.len);
  vbuff_iadd(&tmp, ctx->k_s.data, ctx->k_s.len);
  // qc
  vbuff_iaddu32(&tmp, ctx->q_c.len);
  vbuff_iadd(&tmp, ctx->q_c.data, ctx->q_c.len);
  // qs
  vbuff_iaddu32(&tmp, ctx->q_s.len);
  vbuff_iadd(&tmp, ctx->q_s.data, ctx->q_s.len);
  // k
  // vbuff_iaddu32(&tmp, ctx->k.len); // already formatted
  vbuff_iadd(&tmp, ctx->k.data, ctx->k.len);

  puts("hash material:");
  vbuff_dump(&tmp);

  // hash(aka digest)
  vstr_reserve(h, crypto_hash_sha256_BYTES);
  crypto_hash_sha256((void *)h->data, (void *)tmp.data, tmp.len);
  h->len = crypto_hash_sha256_BYTES; // data full

  puts("hash:");
  vbuff_dump(h);

  // clear
  vstr_clear(&tmp);
}

// s(before packetize)=crypto_sign(data=h,sk=[priv||pub])[:64]
static void ecdh_sign_hash(ssh_context *ctx, vstr_t *h, vstr_t *s) {
  puts("signing hash");
  static uint8_t sk[64];
  memcpy(sk, xample_priv_key, 32);
  memcpy(sk + 32, xample_pub_key, 32);
  // ctx->k_s is blobbed xample_pub_key. use in fut

  vstr_reserve(s, h->len + 64);
  s->len = h->len + 64;
  uint64_t smlen = 0;
  puts("calling crypto_sign...");
  crypto_sign_ed25519(                                     //
      (void *)s->data, &smlen, (void *)h->data, h->len, sk //
  );
  assert(smlen == 64 + 32);
  s->len = 64;               // trim
  memset(sk, 0, sizeof(sk)); // for test, unecessary
  puts("signing done");
}

static void ecdh_derive_keys(ssh_context *ctx) {
  vstr_t tmp;
  vstr_init(&tmp, 0);

  // hash(k||h||a-f||sid)
  vbuff_iadd(&tmp, ctx->k.data, ctx->k.len);
  vbuff_iadd(&tmp, ctx->h.data, ctx->h.len);
  int char_pos = tmp.len;
  vbuff_iaddc(&tmp, 'A');
  vbuff_iadd(&tmp, ctx->sid.data, ctx->sid.len);

  vstr_t *ls_str[6] = {&ctx->iv_c2s,  &ctx->iv_s2c,  &ctx->enc_c2s,
                       &ctx->enc_s2c, &ctx->mac_c2s, &ctx->mac_s2c};

  for (char c = 0; c < 6; c++) {
    tmp.data[char_pos] = c + 'A';
    vstr_reserve(ls_str[c], 64);
    crypto_hash_sha256((void *)ls_str[c]->data, (void *)tmp.data, tmp.len);
  }
  for (char c = 0; c < 6; c++) {
    tmp.len = char_pos;
    vbuff_iadd(&tmp, ls_str[c]->data, 32);
    crypto_hash_sha256(                                           //
        (void *)(ls_str[c]->data + 32), (void *)tmp.data, tmp.len //
    );

    ls_str[c]->len = 64;
    printf("derived key %c:\n", c + 'A');
    vbuff_dump(ls_str[c]);
  }

  // expand to expected bytes?

  // cleanup
  vstr_clear(&tmp);
}