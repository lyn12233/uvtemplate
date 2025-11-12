#include "packet.h"
#include "crypto/chacha.h"
#include "crypto/poly1305.h"
#include "packet_def.h"
#include "port_errno.h"
#include "ssh_context.h"
#include "types/vo.h"

#include "esp/espsock.h"

#include "port_unistd.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "allocator.h"
#include "log.h"

//

int recv_packet(           //
    int sock,              //
    vstr_t *vbuff,         //
    size_t *payload_length //
) {

  // recv packet_length field
  uint32_t packet_length_;
  vstr_t vtmp;
  vstr_init(&vtmp, 0);

  int res = sock_recv(sock, &vtmp, 4, 0);
  if (res < 0) {
    vstr_clear(&vtmp);
    return res;
  }

  // compute packet length
  packet_length_ = *((uint32_t *)vtmp.data);
  packet_length_ = ntohl(packet_length_);
  printf("Debug: total_packet_length = %u\n", packet_length_);

  vstr_clear(&vtmp);
  (void)vtmp;

  // validate packet length
  if (packet_length_ < 1 + 4) { // padding_length(1) + min_padding(4)
    printf("Debug: Invalid packet length (too small)\n");
    return EINVAL;
  }
  if (packet_length_ > SSH_MAX_PACKET_SIZE) {
    return EINVAL;
  }

  // recv complete packet
  vbuff->len = 0;
  res = sock_recv(sock, vbuff, packet_length_, 0);
  if (res < 0) {
    return res;
  }
  if (res < (int)packet_length_) {
    printf("Debug: Incomplete packet data received %d<%d\r\n", res,
           packet_length_);
    return EAGAIN;
  }

  // padding_len
  uint8_t padding_len = vbuff->buff[0]; // 1 byte no endian
  printf("Debug: padding_len = %u, payload_len=%u\r\n", padding_len,
         packet_length_ - padding_len - 1);

  // output length
  if (payload_length)
    *payload_length = packet_length_ - padding_len - 1;

  // validate padding length
  if (padding_len < 4 || padding_len > packet_length_ - 1) {
    printf("Debug: Invalid padding length: %d, %d\n", padding_len,
           packet_length_ - 1);
    return EINVAL;
  }

  return 0;
}

vstr_t *recv_packet_enc(int sock, ssh_context *ctx) {
  printf("recv encrypted packet (%d)...\n",
         ctx->c2s.seq_number); // 0 for the first time
  vstr_t *res = vstr_create(0);

  // build per packet nounce
  uint64_t nounce = ctx->c2s.seq_number;
  ctx->c2s.seq_number++;
  nounce = ntohll(nounce);
  const uint8_t *pnounce = (uint8_t *)&nounce;

  // gen poly key
  uint8_t polykey[32] = {0};
  chacha_ivsetup(&ctx->c2s.ctx_main, pnounce, NULL);
  chacha_encrypt_bytes(&ctx->c2s.ctx_main, polykey, polykey, 32);
  // puts("polykey:");
  // buff_dump(polykey, 32);

  // read 4 bytes: packet length
  chacha_ivsetup(&ctx->c2s.ctx_hdr, pnounce, NULL);
  uint8_t hdr_plain[4];
  vstr_t hdr_recv;
  vstr_init(&hdr_recv, 4); // len=0
  sock_recv(sock, &hdr_recv, 4, 0);
  chacha_encrypt_bytes(         //
      &ctx->c2s.ctx_hdr,        //
      hdr_recv.buff, hdr_plain, //
      4                         //
  );

  // puts("enc packet_len field:");
  // buff_dump(hdr_plain, 4);
  uint32_t len = *((uint32_t *)hdr_plain);
  len = ntohl(len);
  printf("hdr_raw: %u\n", len);
  assert(len < 35000);

  // read packet
  vstr_t main_recv;
  vstr_init(&main_recv, 4 + len + 16);
  memcpy(main_recv.buff, hdr_recv.buff, 4);
  main_recv.len = 4; // recv offs

  vstr_clear(&hdr_recv);
  (void)hdr_recv;

  sock_recv(sock, &main_recv, len + 16, 0);

  // verify first
  {
    const uint8_t *tag = main_recv.buff + 4 + len;
    uint8_t expected_tag[16];
    poly1305_auth(expected_tag, main_recv.buff, len + 4, polykey);

    puts("verifying tag:...");
    // buff_dump(tag, 16);

    buff_dump(expected_tag, 16); // not equal
    assert(memcmp(tag, expected_tag, 16) == 0);
  }
  // dec packet
  const uint8_t one[8] = {1, 0, 0, 0, 0, 0, 0, 0};
  chacha_ivsetup(&ctx->c2s.ctx_main, pnounce, one);
  vstr_reserve(res, len);
  chacha_encrypt_bytes(&ctx->c2s.ctx_main, main_recv.buff + 4, res->buff, len);

  vstr_clear(&main_recv);
  res->len = len;
  // puts("recv enc packet:");
  // vbuff_dump(res);

  return res;
}

void send_packet_enc(int sock, ssh_context *ctx, void *data, uint32_t len) {
  const uint8_t *p = data;
  vstr_t tmp; // data to send
  vstr_init(&tmp, 4 + len + 16);
  puts("enc: sending:...");

  // nounce
  uint64_t nounce = ctx->s2c.seq_number;
  ctx->s2c.seq_number++;
  nounce = ntohll(nounce);
  const uint8_t *pnounce = (uint8_t *)&nounce;

  // gen poly key
  uint8_t polykey[32] = {0};
  chacha_ivsetup(&ctx->s2c.ctx_main, pnounce, NULL);
  chacha_encrypt_bytes(&ctx->s2c.ctx_main, polykey, polykey, 32);
  puts("enc: polykey:...");
  // buff_dump(polykey, 32);

  // enc packet_length (4 bytes)
  uint32_t net_len = htonl(len);
  chacha_ivsetup(&ctx->s2c.ctx_hdr, pnounce, NULL);
  chacha_encrypt_bytes( //
      &ctx->s2c.ctx_hdr, (void *)&net_len, tmp.buff, 4);
  // puts("send: len field:");
  // buff_dump(tmp.data, 4);

  // enc packet
  const uint8_t one[8] = {1, 0, 0, 0, 0, 0, 0, 0};
  chacha_ivsetup(&ctx->s2c.ctx_main, pnounce, one);
  chacha_encrypt_bytes(&ctx->s2c.ctx_main, p, tmp.buff + 4, len);

  // add poly1305 tag/mac
  poly1305_auth( //
      tmp.buff + 4 + len, tmp.buff, 4 + len, polykey);

  // send
  tmp.len = 4 + len + 16;
  sock_send(sock, &tmp, 4 + len + 16, 0);
  vstr_clear(&tmp);
}

//
//
// reprsp_success

void packet_repr(void *payload, size_t payload_len, const vo_type_t *types,
                 size_t nb_types) {
  uint8_t *data = (uint8_t *)payload;
  size_t pos = 0;

  for (size_t i = 0; i < nb_types && pos < payload_len; i++) {
    switch (types[i]) {
    case vot_string: {
      if (pos + 4 > payload_len)
        return;
      uint32_t len = ntohl(*(uint32_t *)(data + pos));
      pos += 4;
      if (pos + len > payload_len)
        return;
      printf("STR[%.*s] ", len, data + pos);
      pos += len;
      break;
    }
    case vot_u8:
      if (pos + 1 > payload_len)
        return;
      printf("U8[%u] ", data[pos]);
      pos += 1;
      break;
    case vot_u32:
      if (pos + 4 > payload_len)
        return;
      printf("U32[%u] ", (int)ntohl(*(uint32_t *)(data + pos)));
      pos += 4;
      break;
    case vot_u64:
      if (pos + 8 > payload_len)
        return;
      printf("U64[%llu] ", ntohll(*(uint64_t *)(data + pos)));
      pos += 8;
      break;
    case vot_bool:
      if (pos + 1 > payload_len)
        return;
      printf("BOOL[%s] ", data[pos] ? "true" : "false");
      pos += 1;
      break;
    case vot_mpint: {
      if (pos + 4 > payload_len)
        return;
      uint32_t len = ntohl(*(uint32_t *)(data + pos));
      pos += 4;
      if (pos + len > payload_len)
        return;
      printf("MPINT[%u bytes] ", len);
      pos += len;
      break;
    }
    case vot_list:
    case vot_namelist: {
      if (pos + 4 > payload_len)
        return;
      uint32_t len = ntohl(*(uint32_t *)(data + pos));
      pos += 4;
      if (pos + len > payload_len)
        return;
      printf("NAMES[%.*s] ", len, data + pos);
      pos += len;
      break;
    }
    }
  }
  printf("\n");
}

vo_t *payload_decode(void *payload, size_t payload_len, const vo_type_t *types,
                     size_t nb_types) {
  vo_t *vo = vo_create(vot_list);
  uint8_t *data = payload;
  int pos = 0;

  for (int i = 0; i < nb_types && pos < payload_len; i++) {
    vo_type_t type = types[i];
    // printf("item %d type %d at pos %d\n", i, (int)type, pos);
    uint32_t len;
    uint8_t val;
    uint32_t val32;
    uint64_t val64;
    vo_t *sub = NULL;

    switch (type) {
    case vot_string:
    case vot_mpint:
    case vot_namelist: {

      if (pos + 4 > payload_len) {
        puts("packet_decode: variable size type requires at least 4 bytes");
        assert(0);
      }

      len = ntohl(*(uint32_t *)(data + pos));
      pos += 4;

      if (pos + len > payload_len) {
        puts("packet decode: variable size data overflows payload edge");
        printf("item %d, pos %d, len %d\r\n", i, pos, len);
        assert(0);
      }

      sub = vo_create(type == vot_string  ? vot_string
                      : type == vot_mpint ? vot_mpint
                                          : vot_namelist);
      vbuff_iadd(&sub->vstr, (void *)(data + pos), len);
      vlist_append(&vo->vlist, sub); // move assign
      free(sub);

      pos += len;

    } break;

    case vot_bool:
    case vot_u8: {
      if (pos + 1 > payload_len) {
        puts("packet decode: overflow; expect u8");
        assert(0);
      }

      val = *(data + pos);
      pos += 1;

      sub = vo_create(type == vot_bool ? vot_bool : vot_u8);
      sub->u8 = val;
      vlist_append(&vo->vlist, sub);
      free(sub);
    } break;

    case vot_u32: {
      if (pos + 4 > payload_len) {
        puts("packet_decode: overflow; expect u32");
        assert(0);
      }

      val32 = ntohl(*(uint32_t *)(data + pos));
      pos += 4;

      sub = vo_create(vot_u32);
      sub->u32 = val32;
      vlist_append(&vo->vlist, sub);
      free(sub);
    } break;
    case vot_u64: {
      if (pos + 8 > payload_len) {
        puts("packet_decode: overflow; expect u64");
        assert(0);
      }

      val64 = ntohll(*(uint64_t *)(data + pos));
      pos += 8;

      sub = vo_create(vot_u64);
      sub->u64 = val64;
      vlist_append(&vo->vlist, sub);
      free(sub);
    } break;
    case vot_list:
      break;
      // default:
    }
  }

  return vo;
}

vstr_t *payload_encode(const vo_t *vo) {
  // vo_t *vo2 = serialize_namelist(vo);
  const vo_t *vo2 = vo;
  // puts("after serialize:");
  // vo_repr(vo2);

  vstr_t *res = vstr_create(0);

  for (int i = 0; i < vo2->vlist.nb; i++) {
    const vo_t *item = &vo2->vlist.data[i];
    switch (item->type) {
    case vot_bool:
    case vot_u8: {
      vbuff_iaddc(res, item->u8);
    } break;
    case vot_u32: {
      vbuff_iaddu32(res, item->u32);
    } break;
    case vot_u64: {
      vbuff_iaddu64(res, item->u64);
    } break;
    case vot_string:
    case vot_mpint:
    case vot_namelist: {
      // printf("packet: encode: got string len: %d\n", item->vstr.len);
      vbuff_iaddu32(res, item->vstr.len);
      vbuff_iadd(res, item->vstr.data, item->vstr.len);
    } break;
    case vot_list: {
      puts("payload encode: do not expect vot_list");
    } break;
      // default:
    }
  }
  // vo_delete(vo2);
  return res;
}

vstr_t *payload2packet(const vstr_t *payload, uint8_t mac_len) {
  vstr_t *res = vstr_create(0);
  vbuff_iaddc(res, 0); // 1 byte: padding len
  vbuff_iadd(res, payload->data, payload->len);

  // calc padding len
  uint8_t pad_len = SSH_DEFAULT_PACKET_ALIGN -
                    (res->len + mac_len) % SSH_DEFAULT_PACKET_ALIGN;
  if (pad_len < 4)
    pad_len += SSH_DEFAULT_PACKET_ALIGN;
  res->data[0] = pad_len;

  // add padding
  for (int i = 0; i < pad_len; i++) {
    vbuff_iaddc(res, i); // should be random
  }

  // printf("created packet len=%u\n", res->len);
  assert((res->len + mac_len) % SSH_DEFAULT_PACKET_ALIGN == 0);
  return res;
}