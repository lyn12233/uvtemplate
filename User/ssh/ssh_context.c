#include "ssh_context.h"
#include "types/vo.h"

#include <stdlib.h>

#include "allocator.h"
#include "log.h"

ssh_context *ssh_context_create() {
  ssh_context *ctx = malloc(sizeof(ssh_context));
  ssh_context_init(ctx);
  return ctx;
}
void ssh_context_init(ssh_context *ctx) {
  vstr_init(&ctx->v_c, 0); // set in exchange_version_header
  vstr_init(&ctx->v_s, 0); // set in exchange_version_header
  vstr_init(&ctx->i_c, 0); // set in consume_kexinit
  vstr_init(&ctx->i_s, 0); // set in send_kexinit
  vstr_init(&ctx->k_s, 0); // set in send_ecdh(pre)
  vstr_init(&ctx->b, 0);   // gen by consume_ecdh_init(post)
  vstr_init(&ctx->q_c, 0); // set by consume_ecdh_init
  vstr_init(&ctx->q_s, 0); // set by consume_ecdh_init(post)
  vstr_init(&ctx->k, 0);   // set by consume_ecdh_init(post)
  vstr_init(&ctx->h, 0);   // set by send_ecdh_reply(pre)

  ctx->first_kex = 1;

  vstr_init(&ctx->sid, 0);     // track first h
  vstr_init(&ctx->iv_c2s, 0);  // set by exchange_newkey(post)
  vstr_init(&ctx->iv_s2c, 0);  // set by exchange_newkey(post)
  vstr_init(&ctx->enc_c2s, 0); // set by exchange_newkey(post)
  vstr_init(&ctx->enc_s2c, 0); // set by exchange_newkey(post)
  vstr_init(&ctx->mac_c2s, 0); // set by exchange_newkey(post)
  vstr_init(&ctx->mac_s2c, 0); // set by exchange_newkey(post)
}

void ssh_context_delete(ssh_context *ctx) {
  ssh_context_clear(ctx);
  free(ctx);
}

void ssh_context_clear(ssh_context *ctx) {
  vstr_clear(&ctx->v_c);
  vstr_clear(&ctx->v_s);
  vstr_clear(&ctx->i_c);
  vstr_clear(&ctx->i_s);
  vstr_clear(&ctx->k_s);
  vstr_clear(&ctx->b);
  vstr_clear(&ctx->q_c);
  vstr_clear(&ctx->q_s);
  vstr_clear(&ctx->k);
  vstr_clear(&ctx->h);
  vstr_clear(&ctx->sid);
  vstr_clear(&ctx->iv_c2s);
  vstr_clear(&ctx->iv_s2c);
  vstr_clear(&ctx->enc_c2s);
  vstr_clear(&ctx->enc_s2c);
  vstr_clear(&ctx->mac_c2s);
  vstr_clear(&ctx->mac_s2c);
}