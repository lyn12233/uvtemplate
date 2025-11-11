#pragma once

#include "ssh_context.h"
#include "types/vo.h"

#include <stddef.h>
#include <stdint.h>

#define SSH_MIN_PACKET_SIZE 16
#define SSH_MAX_PACKET_SIZE 35000

// SSH packet structure (unencrypted)
#pragma pack(push, 1)
typedef struct __attribute__((packed)) {
  uint8_t padding_length; // Padding length
  uint8_t payload[1];     // Variable-length payload followed by padding
} ssh_packet_header_t;
#pragma pack(pop)

int recv_packet(           //
    int sock,              //
    vstr_t *vbuff,         //
    size_t *payload_length //
);

vstr_t *recv_packet_enc(int sock, ssh_context *ctx);
void send_packet_enc(int sock, ssh_context *ctx, void *data, uint32_t len);

vo_t *payload_decode(void *payload, size_t payload_len, const vo_type_t *types,
                     size_t nb_types);

vstr_t *payload_encode(const vo_t *vo);

void packet_repr(void *payload, size_t payload_len, const vo_type_t *types,
                 size_t nb_types);

vstr_t *payload2packet(const vstr_t *payload, uint8_t mac_len);

// vstr_t*packet2payload(const vstr_t);
