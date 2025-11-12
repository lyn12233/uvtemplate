#include "sftp_parse.h"
#include "packet_def.h"
#include "port_socket.h"
#include "portmacro.h"
#include "projdefs.h"
#include "ssh/packet.h"
#include "ssh_context.h"
#include "types/vo.h"

#include "log.h"
#include <stdint.h>

#define DS_LEN 0
#define DS_REG 1

// singelton recv

vstr_t chnldat;
vstr_t pre_send_data;
volatile uint8_t sftp_init_done = 0;
uint8_t dpch_state;
uint32_t dpch_len;
uint32_t dpch_cnt;

void sftp_parser_init() {
  //
  vstr_init(&chnldat, 0);
  vstr_init(&pre_send_data, 0);
  sftp_init_done = 1;
  dpch_state = DS_LEN;
  dpch_len = 4;
  dpch_cnt = 0;
}

void sftp_parse(int sock, ssh_context *ctx, QueueHandle_t q,
                int *should_close) {
  if (!sftp_init_done) {
    *should_close = 0;
    return;
  }

  puts("sftp parser: waiting");
  vstr_t *recvd = recv_packet_enc(sock, ctx);
  puts("sftp recvd:");
  vbuff_dump(recvd);

  uint8_t opcode = recvd->buff[1];
  printf("sftp: parse opcode: %u\r\n", opcode);

  *should_close = 0;
  switch (opcode) {
  case SSH_MSG_CHANNEL_DATA: {
    vo_t *vo = payload_decode(&recvd->buff[1], recvd->len, chnldat_types, 3);
    printf("sftp: data id:%u\r\n", vo->vlist.data[1].u32);
    const vstr_t *pstr = &vo->vlist.data[2].vstr;
    puts("sftp: data:");
    vbuff_dump(pstr);

    if (q) {

      // queue exist, use solo task to parse
      vstr_t vtmp;
      vstr_init(&vtmp, 0);
      vbuff_iadd(&vtmp, pstr->data, pstr->len);

      BaseType_t pass = xQueueSend(q, &vtmp, pdMS_TO_TICKS(2000));
      if (pass != pdPASS) {
        puts("sftp: data block timeout, may be malfunctioning");
        *should_close = 1;
      }

    } else {

      // queue not exist, immediate parse
      for (uint32_t i = 0; i < pstr->len; i++) {
        sftp_dispatch(sock, ctx, pstr->buff[i], should_close);
      }
    } // !q

    vo_delete(vo);
  } break;

  case SSH_MSG_CHANNEL_WINDOW_ADJUST:
    // skip
    break;

  case SSH_MSG_CHANNEL_EOF:
    *should_close = 1;
    break;

  case SSH_MSG_CHANNEL_CLOSE:
    *should_close = 1;
    break;

  case SSH_MSG_KEXINIT:
    puts("key re-exchange not impl, close");
    *should_close = 1;
    break;

  default:
    printf("sftp: warn: unknown message\r\n");
    break;
  }
}

void sftp_dispatch(int sock, ssh_context *ctx, uint8_t c,
                   int *should_close) { //
  printf("dispatch: %x\r\n", c);

  switch (dpch_state) {

  case DS_LEN: {
    vbuff_iaddc(&chnldat, c);
    dpch_cnt++;

    if (dpch_cnt == 4) {
      dpch_len = ntohl(*(uint32_t *)chnldat.buff);
      printf("sftp packet len: %u\r\n", dpch_len);
      vstr_clear(&chnldat);
      dpch_cnt = 0;
      dpch_state = dpch_len != 0 ? DS_REG : DS_LEN;
    }
  } break;

  case DS_REG: {
    vbuff_iaddc(&chnldat, c);
    dpch_cnt++;

    if (dpch_cnt >= dpch_len) {
      sftp_dispatch_spkt(sock, ctx, &chnldat);
      vstr_clear(&chnldat);
      dpch_cnt = 0, dpch_len = 4;
      dpch_state = DS_LEN;
    }
  } break;

  default:
    assert(0);
  } // switch
}

static void sftp_send_pkt(int sock, ssh_context *ctx, const vstr_t *buff);

static void send_version(int sock, ssh_context *ctx) {
  vstr_t buff;
  vstr_init(&buff, 9);

  // vbuff_iaddu32(&buff, 5);
  vbuff_iaddc(&buff, SSH_FXP_VERSION);
  vbuff_iaddu32(&buff, 3);
  sftp_send_pkt(sock, ctx, &buff);

  vstr_clear(&buff);
}

void sftp_dispatch_spkt(int sock, ssh_context *ctx, const vstr_t *pkt) {
  puts("dispatch sftp packet:");
  vbuff_dump(pkt);
  if (pkt->len == 0)
    return;

  switch (pkt->buff[0]) {

  case SSH_FXP_INIT: {
    uint32_t ver = ntohl(*(uint32_t *)(pkt->buff + 1));
    printf("sftp: recv init (version=%u)\r\n", ver);
    send_version(sock, ctx);
    puts("sftp: send version (=3)");
  } break;

  default: {
  } break;
  }
}

///@todo: consider size overflow
static void sftp_send_pkt(int sock, ssh_context *ctx, const vstr_t *buff) {
  vstr_t payload; // ssh payload (channel data)
  vstr_init(&payload, 0);

  vbuff_iaddc(&payload, SSH_MSG_CHANNEL_DATA); // opcode
  vbuff_iaddu32(&payload, 0);                  // recipient id
  vbuff_iaddu32(&payload, buff->len + 4);      // string-len
  vbuff_iaddu32(&payload, buff->len);          // buff(buff.len)
  vbuff_iadd(&payload, buff->data, buff->len); // buff(buff.buff)
  puts("sftp send: payload:");
  vbuff_dump(&payload);

  vstr_t *packet = payload2packet(&payload, 16);

  vstr_clear(&payload);
  (void)payload;

  send_packet_enc(sock, ctx, packet->data, packet->len);

  vstr_delete(packet);
}