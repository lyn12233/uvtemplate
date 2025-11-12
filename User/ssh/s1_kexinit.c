#include "s1_kexinit.h"
#include "esp/espsock.h"
#include "packet_def.h"
#include "ssh/packet.h"

#include "port_unistd.h"
#include "types/vo.h"

#include <stdint.h>
#include <string.h>

#include "log.h"

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
      *done = 0;
      goto dtor;
    }
    if (vtmp.data[0] != '\n' && vtmp.data[0] != '\r')
      vbuff_iaddc(&ctx->v_c, vtmp.data[0]);
    if (vtmp.data[0] == '\n')
      break;
  }
  puts("version header recieved:");
  vbuff_dump(&ctx->v_c);

  *done = 1;

dtor:
  vstr_clear(&vtmp);
}

void send_kexinit(int sock, ssh_context *ctx) {
  vo_t *tmp = vo_create_list_from_il(kexinit_il);
  printf("Debug: sending kexinit (entries %d)\n", tmp->vlist.nb);
  puts("Debug: brief of kex packet s2c:");
  vo_repr(tmp);
  puts("");

  // generate package and save to ctx
  vstr_t *payload = payload_encode(tmp);
  ctx->i_s.len = 0;
  vbuff_iadd(&ctx->i_s, payload->data, payload->len);

  vo_delete(tmp);
  vstr_delete(payload);

  vstr_t *packet = payload2packet(&ctx->i_s, 4);
  printf("Debug: created packet (size: %d)\n", packet->len);

  // send
  uint32_t len_to_send = htonl(packet->len);
  const vstr_t vlen = (vstr_t){.len = 4, .buff = (void *)&len_to_send};
  sock_send(sock, &vlen, 4, 0);
  sock_send(sock, packet, packet->len, 0);
  puts("kexinit: sent");

  vstr_delete(packet);
}
void consume_kexinit(int sock, ssh_context *ctx) {
  vstr_t vbuff;
  vstr_init(&vbuff, 0);
  uint32_t payload_len;
  int res;

  res = recv_packet(sock, &vbuff, &payload_len);
  if (res < 0) {
    puts("conusme kexinit: can not recv packet");
    vstr_clear(&vbuff);
    return;
  }

  // store and represent
  vstr_clear(&ctx->i_c);
  vbuff_iadd(&ctx->i_c, &vbuff.data[1], payload_len);
  vstr_clear(&vbuff);

  // puts("I_c:");
  // vbuff_dump(&ctx->i_c);

  // repr
  vo_t *tmp = payload_decode(      //
      &ctx->i_c.data, payload_len, //
      kex_types, LEN_KEX_TYPES     //
  );
  printf("packet received length %d, payload:\n", (int)payload_len);
  vo_repr(tmp);
  vo_delete(tmp);
  puts("");
}
