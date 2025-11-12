#include "packet_def.h"
#include "types/vo.h"
#include <stdint.h>

const vo_type_t kex_types[29] = {
    vot_u8, // message type (20)

    vot_u8,       vot_u8, vot_u8, vot_u8,
    vot_u8,       vot_u8, vot_u8, vot_u8, // cookie[0-7]
    vot_u8,       vot_u8, vot_u8, vot_u8,
    vot_u8,       vot_u8, vot_u8, vot_u8, // cookie[8-15]

    vot_namelist, // kex_algorithms
    vot_namelist, // server_host_key_algorithms
    vot_namelist, // encryption_algorithms_client_to_server
    vot_namelist, // encryption_algorithms_server_to_client
    vot_namelist, // mac_algorithms_client_to_server
    vot_namelist, // mac_algorithms_server_to_client
    vot_namelist, // compression_algorithms_client_to_server
    vot_namelist, // compression_algorithms_server_to_client
    vot_namelist, // languages_client_to_server
    vot_namelist, // languages_server_to_client
    vot_bool,     // first_kex_packet_follows
    vot_u32,      // reserved
};
const struct vo_initializer kexinit_il[30] = {
    {.type = vot_u8, .u8 = 20},
    {.type = vot_u8, .u8 = 11}, // cookie[0]
    {.type = vot_u8, .u8 = 22},
    {.type = vot_u8, .u8 = 20},
    {.type = vot_u8, .u8 = 20},
    {.type = vot_u8, .u8 = 20},
    {.type = vot_u8, .u8 = 20},
    {.type = vot_u8, .u8 = 20},
    {.type = vot_u8, .u8 = 20},
    {.type = vot_u8, .u8 = 20}, // cookie[8]
    {.type = vot_u8, .u8 = 20},
    {.type = vot_u8, .u8 = 20},
    {.type = vot_u8, .u8 = 20},
    {.type = vot_u8, .u8 = 20},
    {.type = vot_u8, .u8 = 20},
    {.type = vot_u8, .u8 = 20},
    {.type = vot_u8, .u8 = 20},
    {.type = vot_namelist, .str = "curve25519-sha256"}, // kex
    {.type = vot_namelist, .str = "ssh-ed25519"},       // server auth

    {
        .type = vot_namelist,
        .str = "chacha20-poly1305@openssh.com",
    }, // cipher c2s
    {
        .type = vot_namelist,
        .str = "chacha20-poly1305@openssh.com",
    }, // cipher s2c

    {.type = vot_namelist, .str = "none"}, // mac c2s
    {.type = vot_namelist, .str = "none"}, // mac s2c
    {.type = vot_namelist, .str = "none"}, // zip c2s
    {.type = vot_namelist, .str = "none"}, // zip s2c
    {.type = vot_namelist, .str = ""},     // lang
    {.type = vot_namelist, .str = ""},     // lang
    {.type = vot_bool, .u8 = 0},           // first key follows
    {.type = vot_u32, .u32 = 0},           // reserved
    {.type = vot_string, .str = NULL},     // termination

};

const vo_type_t ecdhinit_types[2] = {
    vot_u8,
    vot_string, // q_c
};

const vo_type_t service_request_types[2] = {
    vot_u8,
    vot_string,
};
const struct vo_initializer service_accept_il[3] = {
    {.type = vot_u8, .u8 = SSH_MSG_SERVICE_ACCEPT},
    {.type = vot_string, .str = "ssh-userauth"},
    {.type = vot_string, .str = NULL},
};
const vo_type_t userauth_pubkey_types[8] = {
    vot_u8,     //
    vot_string, // user_name
    vot_string, // service_name
    vot_string, // method_name, len(base)=4
    vot_u8,     // has_sig
    vot_string, // alg_name
    vot_string, // pubkey, len(check)=7
    vot_string, // blobbed sig, len(full)=8
};

//
//

const vo_type_t chnlop_types[5] = {
    vot_u8,  vot_string,
    vot_u32, // sneder channel
    vot_u32, // wind sz
    vot_u32, // max pkt sz
};
const struct vo_initializer chnlop_cfm[5 + 1] = {
    {.type = vot_u8, .u8 = SSH_MSG_CHANNEL_OPEN_CONFIRMATION},
    {.type = vot_u32, .u32 = 0},        // recv chnl
    {.type = vot_u32, .u32 = 0},        // send chnl
    {.type = vot_u32, .u32 = 33554432}, // wind sz
    {.type = vot_u32, .u32 = 2000},     // max pkt sz
    {.type = vot_string, .str = NULL},
};
const vo_type_t chnlreq_types[4] = {
    vot_u8,     // opc
    vot_u32,    // recp chnl
    vot_string, // req name
    vot_u8,     // want reply
};
const vo_type_t chnlreq_subsys_types[5] = {
    vot_u8,     // opc
    vot_u32,    // recp chnl
    vot_string, // req name
    vot_u8,     // want reply
    vot_string, // subsystem name
};
const struct vo_initializer chnlsuc[2 + 1] = {
    {.type = vot_u8, .u8 = SSH_MSG_CHANNEL_SUCCESS},
    {.type = vot_u32, .u32 = 0},
    {.type = vot_string, .str = NULL},
};

//
//

const vo_type_t chnldat_types[3] = {
    vot_u8,
    vot_u32,    // recp id
    vot_string, // data
};

//
//
//

const uint8_t xample_priv_key[32] = {
    0xD0, 0x63, 0x16, 0xEB, 0x03, 0x8A, 0xCD, 0x72, 0xC1, 0x91, 0x17,
    0x89, 0x6F, 0xD5, 0x8F, 0x39, 0xA3, 0x73, 0xCB, 0xEB, 0x26, 0x54,
    0x3C, 0x8E, 0x42, 0x5F, 0xD9, 0x48, 0x44, 0xF7, 0x3B, 0xE3,
};

const uint8_t xample_pub_key[32] = {
    0xec, 0xfd, 0xd7, 0x98, 0x3d, 0xa8, 0x02, 0x50, 0xb3, 0xff, 0x77,
    0x74, 0xa4, 0x6d, 0x06, 0x19, 0xb4, 0x5f, 0x0d, 0x29, 0x9e, 0x3d,
    0x6a, 0x78, 0xc3, 0xfe, 0xf0, 0xaa, 0x9c, 0x74, 0xd8, 0x69,
};
