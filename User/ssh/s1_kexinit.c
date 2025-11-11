#include "s1_kexinit.h"
#include "esp/espsock.h"

#include <stdint.h>
#include <string.h>

#include "log.h"
#include "types/vo.h"

void exchange_version_header(int sock, ssh_context *ctx, int *done) {
  // send
  char *my_version = "SSH-2.0-NotASSH NotADescription\r\n";
  vstr_t v_version = (vstr_t){.data = my_version, .len = strlen(my_version)};
  sock_send(sock, &v_version, strlen(my_version), 0);
  ctx->v_s.len = 0;
  vbuff_iadd(&ctx->v_s, my_version, strlen(my_version) - 2);

  // receive version header and store

  vstr_t vtmp; // recvbyte
  vstr_init(&vtmp, 1);
  // clear v_c
  ctx->v_c.len = 0;

  while (1) {
    vtmp.len = 0;
    if (sock_recv(sock, &vtmp, 1, 0) < 0) {
      debug2("version header: recv error\r\n");
      break;
    }
    if (vtmp.data[0] != '\n' && vtmp.data[0] != '\r')
      vbuff_iaddc(&ctx->v_c, vtmp.data[0]);
    if (vtmp.data[0] == '\n')
      break;
  }
  puts("version header recieved:");
  vbuff_dump(&ctx->v_c);

  *done = 1;
}
void send_kexinit(int sock, ssh_context *ctx) {}
void consume_kexinit(int sock, ssh_context *ctx) {}
