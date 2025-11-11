#pragma once

#include "packet.h"
#include "types/vo.h"

#define SSH_MSG_SERVICE_REQUEST 5
#define SSH_MSG_SERVICE_ACCEPT 6
#define SSH_MSG_KEXINIT 20
#define SSH_MSG_NEWKEYS 21

#define SSH_MSG_KEX_ECDH_INIT 30
#define SSH_MSG_KEX_ECDH_REPLY 31

#define SSH_MSG_USERUATH_REQUEST 50
#define SSH_MSG_USERUATH_FAILURE 51
#define SSH_MSG_USERUATH_SUCCESS 52
#define SSH_MSG_USERUATH_BANNER 53

#define LEN_KEX_TYPES 29
#define LEN_ECHDINIT_TYPES 2

#define SSH_DEFAULT_PACKET_ALIGN 8 // for stream cipher

extern const vo_type_t kex_types[29];
extern const struct vo_initializer kexinit_il[30];

extern const vo_type_t ecdhinit_types[2];

extern const vo_type_t service_request_types[2];
extern const struct vo_initializer service_accept_il[3];
extern const vo_type_t userauth_pubkey_types[8];

// typedef struct {
//   enum {
//     kex_curve25519_sha256,
//   } kex;
//   enum {
//     auth_ssh_ed25519,
//   } auth;
//   enum {
//     cipher_chacha20_poly1305,
//   } cipher;
//   enum {
//     mac_none,
//   } mac;
//   enum {
//     zip_none,
//     zip_use_zlib,
//   } zip;
// } alg_context_t;

extern const uint8_t xample_priv_key[32];
extern const uint8_t xample_pub_key[32];