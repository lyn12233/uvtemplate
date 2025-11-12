#include "s4_openchnl.h"

#include "packet.h"
#include "packet_def.h"

#include "log.h"
#include "ssh/packet.h"
#include "types/vo.h"

void consume_openchannel(int sock, ssh_context *ctx, int *done) {
  vstr_t *recvd = recv_packet_enc(sock, ctx);

  if (!(recvd->len >= 2 && recvd->buff[1] == SSH_MSG_CHANNEL_OPEN)) {
    puts("channel: not as expected (channel_open):");
    vbuff_dump(recvd);
    *done = 0;
    vstr_delete(recvd);
    return;
  }

  vo_t *vo = payload_decode(&recvd->buff[1], recvd->len, chnlop_types, 5);

  vstr_delete(recvd);
  (void)recvd;

  const vstr_t *pstr = &vo->vlist.data[1].vstr;
  printf("expected channel type session: %.*s\r\n", pstr->len, pstr->data);
  if (pstr->len == 7 && strncmp("session", pstr->data, 7) == 0) {
    *done = 1;
    puts("expected");
  } else {
    *done = 0;
    puts("unexpected");
    vo_delete(vo);
    return;
  }

  vo_delete(vo);

  // send confirmation

  vo = vo_create_list_from_il(chnlop_cfm);
  vstr_t *payload = payload_encode(vo);
  vstr_t *packet = payload2packet(payload, 16);

  send_packet_enc(sock, ctx, packet->data, packet->len);

  vstr_delete(packet);
  vstr_delete(payload);
  vo_delete(vo);

  puts("channel open confirmation sent");
}

void respond_chnlreq(int sock, ssh_context *ctx, int *done) {
  puts("channel: respond channel request (expected subsystem-sftp)");
  vstr_t *recvd = recv_packet_enc(sock, ctx);

  if (!(recvd->len >= 2 && recvd->buff[1] == SSH_MSG_CHANNEL_REQUEST)) {
    puts("channel: not as expected (channel_request):");
    vbuff_dump(recvd);
    *done = 0;
    vstr_delete(recvd);
    return;
  }

  vo_t *vo = payload_decode(&recvd->buff[1], recvd->len, chnlreq_types, 4);

  const vstr_t *pstr = &vo->vlist.data[2].vstr;
  puts("request name:");
  printf("%.*s\r\n", pstr->len, pstr->data);
  if (pstr->len == 9 && strncmp("subsystem", pstr->data, 9) == 0) {
    *done = 1;
    puts("expected");
  } else {
    *done = 0;
    vo_delete(vo);
    vstr_delete(recvd);
    puts("unexpected");
    return;
  }

  // re-decode
  vo_delete(vo);
  vo = payload_decode(&recvd->buff[1], recvd->len, chnlreq_subsys_types, 5);

  vstr_delete(recvd);
  (void)recvd;

  int want_reply = vo->vlist.data[3].u8;
  printf("want reply: %d\n", want_reply);

  puts("subsystem name:");
  pstr = &vo->vlist.data[4].vstr;
  printf("%.*s\n", pstr->len, pstr->data);
  if (pstr->len == 4 && strncmp("sftp", pstr->data, 4) == 0) {
    *done = 1;
    puts("expected");
  } else {
    *done = 0;
    vo_delete(vo);
    puts("unexpected");
    return;
  }

  vo_delete(vo);

  // reply

  if (want_reply) {
    // respond success
    vo = vo_create_list_from_il(chnlsuc);
    puts("channel sending success reply:");
    vo_repr(vo);
    puts("");
    vstr_t *payload = payload_encode(vo);
    vstr_t *packet = payload2packet(payload, 16);

    send_packet_enc(sock, ctx, packet->data, packet->len);

    vstr_delete(packet);
    vstr_delete(payload);
    vo_delete(vo);
    puts("channel req success sent");
  }

  *done = 1;

  // test
  // vstr_t *packet = recv_packet_enc(sock, ctx);
  // puts("next:");
  // vbuff_dump(packet);
  // vstr_delete(packet);
}