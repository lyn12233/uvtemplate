#include "s1_kexinit.h"
#include "s2_ecdh_init.h"
#include "s3_userauth.h"
#include "s4_openchnl.h"
#include "sftp_parse.h"
#include "sftp_task.h"

#include "ssh/s1_kexinit.h"
#include "ssh/ssh_context.h"

#include "esp/espsock.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

#include <stdint.h>

#include "log.h"
#include "types/vo.h"

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
  if (!done) {
    res = -5;
    goto dtor;
  }

  {
    int half;
    puts("stage3(2): conduct userauth");
    respond_userauth(sock, ctx, &done, &half);
    if (!done) {
      res = -6;
      goto dtor;
    }
  }

  puts("stage4(1): open channel");
  consume_openchannel(sock, ctx, &done);
  if (!done) {
    res = -7;
    goto dtor;
  }

  puts("stage4(2): consume channel request(subsystem-sftp)");
  respond_chnlreq(sock, ctx, &done);
  if (!done) {
    res = -8;
    goto dtor;
  }

  puts("stage5(1): sftp init");
  // QueueHandle_t q = xQueueCreate(1, sizeof(vstr_t));
  // puts("sftp stream queue created");
  // sftp_start_task(q);
  // puts("sftp task started");
  sftp_parser_init();

  int should_close = 0;

  puts("stage5(2): sftp loop");
  while (!should_close) {
    if (!sock_is_conn(sock)) {
      puts("sshd_acpt: socket closed by the client");
      res = -9;
      goto dtor;
    }
    sftp_parse(sock, ctx, NULL, &should_close);
  }
  res = 0;

  puts("stage6:cleanup");

dtor:
  printf("sshd_acpt: returning %d\r\n", res);
  ssh_context_delete(ctx);
  return res;
}