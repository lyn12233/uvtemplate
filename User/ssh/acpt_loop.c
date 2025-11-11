#include "esp/espsock.h"
#include "s1_kexinit.h"
#include "ssh/s1_kexinit.h"
#include "ssh/ssh_context.h"

#include <stdint.h>

#include "log.h"

uint8_t sshd_acpt(int sock) {
  int done = 0;
  uint8_t res = 0;
  ssh_context *ctx = ssh_context_create();

  //
  puts("stage1: recv version header v_c");
  exchange_version_header(sock, ctx, &done);
  if (!done) {
    res = -1;
    goto dtor;
  }

dtor:
  ssh_context_delete(ctx);
  return res;
}