#include "esp/espsock.h"
#include "s1_kexinit.h"
#include "s2_ecdh_init.h"
#include "s3_userauth.h"
#include "ssh/s1_kexinit.h"
#include "ssh/ssh_context.h"

#include <stdint.h>

#include "log.h"

uint8_t sshd_acpt(int sock) {
  int done = 0;
  uint8_t res = 0;
  ssh_context *ctx = ssh_context_create();

  //
  puts("stage1(1): recv version header v_c");
  exchange_version_header(sock, ctx, &done);
  if (!done) {
    res = -1;
    goto dtor;
  }

  puts("stage1(2): exchange kexinit");
  send_kexinit(sock, ctx);
  consume_kexinit(sock, ctx);

  puts("stage2(1): receive ecdh init");
  done = 0;
  consume_ecdh_init(sock, ctx, &done);
  if (!done) {
    res = -2;
    goto dtor;
  }

  puts("stage2(2): reply ecdh");
  send_ecdh_reply(sock, ctx, &done);
  if (!done) {
    res = -3;
    goto dtor;
  }

  puts("stage2(3): exchange msg-newkey");
  exchange_msg_newkey(sock, &done);
  if (!done) {
    res = -4;
    goto dtor;
  }

  puts("stage3(1): respond svc req");
  respond_auth_service(sock, ctx, &done);

dtor:
  ssh_context_delete(ctx);
  return res;
}