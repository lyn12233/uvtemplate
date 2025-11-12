#include "s3_userauth.h"
#include "packet.h"
#include "packet_def.h"
#include "types/vo.h"

#include "log.h"

static void send_userauth_success(int sock, ssh_context *ctx);

void respond_auth_service(int sock, ssh_context *ctx, int *done) {
  vstr_t *recvd = recv_packet_enc(sock, ctx);
  if (!(recvd->len > 2 && recvd->data[1] == SSH_MSG_SERVICE_REQUEST)) {
    puts("useratuh: not expected SSH_MSG_SERVICE_REQUEST:");
    vbuff_dump(recvd);
    *done = 0;
    vstr_delete(recvd);
    return;
  }

  vo_t *vo = payload_decode(                                //
      recvd->data + 1, recvd->len, service_request_types, 2 //
  );

  vstr_delete(recvd);
  (void)recvd;

  const vstr_t *method = &vo->vlist.data[1].vstr;
  printf("expect ssh-userauth ? %.*s\r\n", method->len, method->data);
  if (method->len == 12 && memcmp(method->data, "ssh-userauth", 12) == 0) {
    puts("as expected");
  } else {
    puts("unexpected service");
    *done = 0;
    vo_delete(vo);
    return;
  }

  vo_delete(vo);

  // send

  vo = vo_create_list_from_il(service_accept_il);
  vstr_t *payload = payload_encode(vo);

  vo_delete(vo);
  (void)vo;

  vstr_t *packet = payload2packet(payload, 16);

  vstr_delete(payload);
  (void)payload;

  puts("userauth: send packet:");
  vbuff_dump(packet);
  send_packet_enc(sock, ctx, packet->data, packet->len);

  *done = 1;

  vstr_delete(packet);
}

void respond_userauth(int sock, ssh_context *ctx, int *done, int *half) {
  vstr_t *recvd = recv_packet_enc(sock, ctx);
  if (!(recvd->len > 2 && recvd->data[1] == SSH_MSG_USERUATH_REQUEST)) {
    puts("useratuh: not as expected (SSH_MSG_USERAUTH_REQUEST):");
    vbuff_dump(recvd);
    *done = 0;
    vstr_delete(recvd);
    return;
  }

  vo_t *vo = payload_decode(                                //
      recvd->data + 1, recvd->len, userauth_pubkey_types, 4 //
  );

  vstr_delete(recvd);
  (void)recvd;

  const vstr_t *method = &vo->vlist.data[3].vstr;
  printf("userauth: method: %.*s", method->len, method->data);
  if (memcmp(method->data, "none", 4) == 0) {
    puts("supported");
    send_userauth_success(sock, ctx);
    *done = 1;
  } else {
    puts("not supported");
    *done = 0;
  }

  vo_delete(vo);
}

static void send_userauth_success(int sock, ssh_context *ctx) {
  vstr_t *tmp = vstr_create(0);
  vbuff_iaddc(tmp, SSH_MSG_USERUATH_SUCCESS);
  vstr_t *packet = payload2packet(tmp, 16);
  send_packet_enc(sock, ctx, packet->data, packet->len);

  vstr_delete(tmp);
  vstr_delete(packet);
  puts("userauth success sent");
}