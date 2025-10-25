///@file vo.h
///@brief variant object
#pragma once

#include <stdint.h>

///@defgroup types_vo
///@brief variant object to represent ssh binary packet format
///@details see RFC 4253 chapter 6 "Binary Packet Protocol" and RFC 4251 chapter
/// 5 "Data Type Representations Used in the SSH Protocols"
///@{

/// max chars in a string(excluding), note packet size limit is 2^32-1
#define VO_STRMAX ((1ull << 17) - 1ull)
/// max entries in a list of vo's(excluding), note packet size limit is 2^32-1
#define VO_LSTMAX ((1ull << 17) - 1ull)

/// types defined in rfc 4251
typedef enum {
  vot_bool,
  vot_u8,
  vot_u32,
  vot_u64,
  vot_string,
  vot_mpint,
  vot_list,
} vo_type_t;

///@struct vstr_t
///@brief variable length string, also used as buff
typedef struct {
  uint32_t len : 24;
  uint32_t reserved : 24;
  union {
    char *data;
    uint8_t *buff;
  };
} vstr_t;

///@struct vlist_t
///@brief variable length list of vo_t
typedef struct {
  uint32_t nb : 24;
  uint32_t reserved : 24;
  struct vo_s *data;
} vlist_t;

///@struct vo_t
///@brief variant object
struct vo_s {
  vo_type_t type;
  union {
    uint8_t bool;
    uint8_t u8;
    uint32_t u32;
    uint64_t u64;
    vstr_t vstr;
    vstr_t mpint;
    vlist_t vlist;
  };
};
typedef struct vo_s vo_t;

vstr_t *vstr_create(uint32_t resv);
void vstr_delete(vstr_t *str);
void vstr_init(vstr_t *str, uint32_t resv);
void vstr_clear(vstr_t *str);
void vstr_reserve(vstr_t *str, uint32_t resv);

void vstr_copy(const vstr_t *src, vstr_t *dst);
uint32_t vstr_strlen(vstr_t *str);
const char *vstr_cstr(vstr_t *str);
void vstr_iaddpc(vstr_t *str, const char *sub);
void vbuff_iaddc(vstr_t *str, char c);
void vbuff_iaddu32(vstr_t *str, uint32_t c);
void vbuff_iaddu64(vstr_t *str, uint64_t c);
void vbuff_iadd(vstr_t *str, const char *pc, uint32_t len);

vlist_t *vlist_create(uint32_t resv);
void vlist_delete(vlist_t *list);
void vlist_init(vlist_t *list, uint32_t len);
void vlist_clear(vlist_t *list);
void vlist_reserve(vlist_t *list, uint32_t resv);

void vlist_copy(const vlist_t *src, vlist_t *dst);
void vlist_append(vlist_t *list, vo_t *vo);

vo_t *vo_create(vo_type_t type);
void vo_delete(vo_t *vo);
void vo_init(vo_t *vo, vo_type_t type);
void vo_clear(vo_t *vo);

void vo_copy(const vo_t *src, vo_t *dst);
vstr_t *vo_tobuff(const vo_t *vo);

///@}