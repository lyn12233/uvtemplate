#include "vo.h"

#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "allocator.h"
#include "log.h"

#define ALLOC_ERROR 1
#define UNREACHABLE 1 // WTF

// variable length string

// behavior notes: all manipulation ensures null terminated or NULL; all
// parameters are assumed valid(not NULL); for string, always reserved>=len+1
// or data=NULL; data=NULL equals to reserved=0, induces len=0, reserved equals
// size of data; always reserved < STRMAX(1<<20);

// str functions can be used for buff which has less restrictions. at this point
// the output is valid-as-buff but may not be null-terminated. str is always
// valid buff; to transfrom buff to str, use vstr_iaddpc(str,"")

// _clear is used to clear all dynamically allocated elements of a struct, so
// that deleting struct* only needs freeing itself; alternatively to clear the
// content of a str/buff, directly set len=0

vstr_t *vstr_create(uint32_t resv) {
  vstr_t *res = malloc(sizeof(vstr_t));
  assert(res && ALLOC_ERROR);
  vstr_init(res, resv);
  return res;
}
void vstr_delete(vstr_t *str) {
  vstr_clear(str);
  free(str);
}
void vstr_init(vstr_t *str, uint32_t resv) {
  str->len = 0;
  str->reserved = resv;
  if (resv) {
    assert(resv < VO_STRMAX);
    str->data = malloc(resv);
    str->data[0] = 0;
  } else {
    str->data = NULL;
  }
}
void vstr_clear(vstr_t *str) {
  str->len = 0;
  str->reserved = 0;
  if (str->data)
    free(str->data);
  str->data = NULL;
}
void vstr_reserve(vstr_t *str, uint32_t resv) {
  if (resv <= str->reserved)
    return;
  resv = resv * 3 / 2;
  assert(resv < VO_STRMAX);
  char *p2 = malloc(resv);
  p2[str->len] = 0;
  if (str->data) {
    memcpy(p2, str->data, str->len);
    free(str->data);
  }
  str->reserved = resv;
  str->data = p2;
}

void vstr_copy(const vstr_t *src, vstr_t *dst) {
  vstr_reserve(dst, src->len + 1);
  if (src->data && dst->data) {
    memcpy(dst->data, src->data, src->len);
    dst->data[src->len] = 0;
  }
  dst->len = src->len;
}
uint32_t vstr_strlen(vstr_t *str) {
  if (str->data) {
    return strnlen(str->data, str->len);
  }
  return 0;
}
const char *vstr_cstr(vstr_t *str) {
  if (str->data) {
    return str->data;
  }
  static const char *nstr = "\0";
  return nstr;
}
void vstr_iaddpc(vstr_t *str, const char *sub) {
  uint32_t l1 = str->len, l2 = strnlen(sub, VO_STRMAX);
  assert(l1 + l2 + 1 < VO_STRMAX);
  vstr_reserve(str, l1 + l2 + 1);
  memcpy(&str->data[l1], sub, l2);
  str->len = l1 + l2;
  str->data[l1 + l2] = 0;
}

// variable length buffer
// similar to vlist, but do not ensure null termination and containing null

void vbuff_iaddc(vstr_t *str, char c) {
  assert(str->len + 1 < VO_STRMAX);
  vstr_reserve(str, str->len + 1);
  str->data[str->len] = c;
  str->len++;
}
void vbuff_iaddu32(vstr_t *str, uint32_t c) {
  for (int i = 3; i >= 0; i--) {
    vbuff_iaddc(str, (c >> (i * 8)) & 0xff);
  }
}
void vbuff_iaddu64(vstr_t *str, uint64_t c) {
  for (int i = 7; i >= 0; i--) {
    vbuff_iaddc(str, (c >> (i * 8)) & 0xff);
  }
}
void vbuff_iadd(vstr_t *str, const char *pc, uint32_t len) {
  assert(str->len + len < VO_STRMAX);
  uint32_t new_len = str->len + len;
  if (!new_len)
    return; // ensure data not null
  vstr_reserve(str, new_len);
  memcpy(&str->data[str->len], pc, len);
  str->len = new_len;
}
void vbuff_iaddmp(vstr_t *str, const char *pc, uint32_t len) {
  // rfc425x compatible mpint packaging
  // dismiss
  while (len > 0 && *pc == 0) {
    pc++, len--;
  }
  uint8_t prepend = (len > 0 && (pc[0] & 0x80) != 0) ? 1 : 0;
  // write length (no overflow for len \ll MAX_UINT; no clear)
  vbuff_iaddu32(str, len + prepend);
  if (prepend) {
    vbuff_iadd(str, "\0", 1);
  }
  if (len != 0) {
    vbuff_iadd(str, pc, len);
  }
}

void vbuff_dump(const vstr_t *str) {
  printf("buffer (len=%d):\n", str->len);
  buff_dump(str->data, str->len);
}
void buff_dump(const void *data, uint32_t len) {
  const uint8_t *p = data;
  for (size_t i = 0; i < len; i += 16) {
    printf("%.4zu:", i);
    for (size_t j = i; j < i + 16; j++) {
      if (j < len)
        printf("%02x ", p[j]);
      else
        printf("   ");
    }
    printf(" ");
    for (size_t j = i; j < i + 16 && j < len; j++) {
      if (p[j] < 128 && isprint(p[j])) {
        printf("%c", p[j]);
      } else {
        printf(".");
      }
    }
    puts("");
  }
}

// variable length list(array)
// behavior notes: all parameters are assumed valid(not NULL); always
// reserved>=len or data=NULL; data=NULL equals to reserved=0, equals len=0,
// reserved equals size of data times size of vo_t; always reserved <
// LSTMAX(1<<20); any item in list is vo_t;

vlist_t *vlist_create(uint32_t resv) {
  vlist_t *res = malloc(sizeof(vlist_t));
  assert(res && ALLOC_ERROR);
  vlist_init(res, resv);
  return res;
}
void vlist_delete(vlist_t *list) {
  vlist_clear(list);
  free(list);
}
void vlist_init(vlist_t *list, uint32_t resv) {
  list->nb = 0;
  list->reserved = resv;
  if (resv) {
    assert(resv < VO_LSTMAX);
    list->data = malloc(sizeof(vo_t) * resv);
  } else {
    list->data = NULL;
  }
}
void vlist_clear(vlist_t *list) {
  for (int i = 0; i < list->nb; i++) {
    vo_clear(&list->data[i]);
  }
  list->nb = 0;
  list->reserved = 0;
  if (list->data)
    free(list->data);
  list->data = NULL;
}
void vlist_reserve(vlist_t *list, uint32_t resv) {
  if (resv <= list->reserved)
    return;
  resv = resv * 3 / 2;
  assert(resv < VO_LSTMAX);
  vo_t *p2 = malloc(sizeof(vo_t) * resv);
  if (list->data) {
    memcpy(p2, list->data, sizeof(vo_t) * list->nb);
    free(list->data);
    // move assign, no hanging-pointer here
  }
  list->reserved = resv;
  list->data = p2;
}

void vlist_copy(const vlist_t *src, vlist_t *dst) {
  vlist_reserve(dst, src->nb);
  if (src->data) {
    for (int i = 0; i < src->nb; i++) {
      vo_init(&dst->data[i], src->data[i].type);
      vo_copy(&src->data[i], &dst->data[i]);
    }
  }
  dst->nb = src->nb;
}
void vlist_append(vlist_t *list, vo_t *vo) {
  // vo: move assign; if vo is created by vo_create, remember to free(vo)
  vlist_reserve(list, list->nb + 1);
  // memcpy(&list->data[list->nb], vo, sizeof(vo_t));
  list->data[list->nb] = *vo;
  list->nb++;
}

vo_t *vo_create(vo_type_t type) {
  vo_t *res = malloc(sizeof(vo_t));
  assert(res && ALLOC_ERROR);
  vo_init(res, type);
  return res;
}
void vo_delete(vo_t *vo) {
  vo_clear(vo);
  free(vo);
}
void vo_init(vo_t *vo, vo_type_t type) {
  vo->type = type;
  switch (type) {
  case vot_bool:
  case vot_u8:
  case vot_u32:
  case vot_u64:
    vo->u64 = 0;
    break;
  case vot_string:
  case vot_mpint:
  case vot_namelist:
    vstr_init(&vo->vstr, 0);
    break;
  case vot_list:
    vlist_init(&vo->vlist, 0);
    break;
  default:
    assert(0 && UNREACHABLE);
  }
}
void vo_clear(vo_t *vo) {
  switch (vo->type) {
  case vot_bool:
  case vot_u8:
  case vot_u32:
  case vot_u64:
    vo->u64 = 0;
    break;
  case vot_string:
  case vot_mpint:
  case vot_namelist:
    vstr_clear(&vo->vstr);
    break;
  case vot_list:
    vlist_clear(&vo->vlist);
    break;
  default:
    assert(0 && UNREACHABLE);
  }
}

static uint8_t is_name_list(const vo_t *vo) {
  if (vo->type != vot_list)
    return 0;
  for (int i = 0; i < vo->vlist.nb; i++) {
    if (vo->vlist.data[i].type != vot_string)
      return 0;
  }
  return 1;
}

static uint8_t is_packet(const vo_t *vo) {
  if (vo->type != vot_list)
    return 0;
  for (int i = 0; i < vo->vlist.nb; i++) {
    if (vo->vlist.data[i].type == vot_list && !is_name_list(&vo->vlist.data[i]))
      return 0;
  }
  return 1;
}

void vo_copy(const vo_t *src, vo_t *dst) {
  vo_clear(dst);
  dst->type = src->type;
  switch (src->type) {
  case vot_bool:
  case vot_u8:
    dst->u8 = src->u8;
    break;
  case vot_u32:
    dst->u32 = src->u32;
    break;
  case vot_u64:
    dst->u64 = src->u64;
    break;
  case vot_string:
  case vot_mpint:
  case vot_namelist:
    vstr_copy(&src->vstr, &dst->vstr);
    break;
  case vot_list:
    vlist_copy(&src->vlist, &dst->vlist);
    break;
  default:
    assert(0 && UNREACHABLE);
  }
}

void vo_repr(const vo_t *vo) {
  printf("{");
  switch (vo->type) {
  case vot_bool: {
    printf("\"bool\":%s, ", vo->bool ? "true" : "false");
  } break;
  case vot_u8: {
    printf("\"u8\":%u, ", vo->u8);
  } break;
  case vot_u32: {
    printf("\"u32\":%u, ", vo->u32);
  } break;
  case vot_u64: {
    printf("\"u64\":%llu, ", vo->u64);
  } break;
  case vot_mpint: {
    printf("\"mpint\":\"");
    for (int i = 0; i < vo->vstr.len; i++) {
      printf("%02X ", vo->vstr.data[i]);
    }
    printf("\", ");
  } break;
  case vot_namelist:
  case vot_string: {
    printf("\"%s[%d]\":\"", vo->type == vot_namelist ? "namelist" : "string",
           vo->vstr.len);
    printf("%.*s\", ", vo->vstr.len, vo->vstr.data);
  } break;
  case vot_list: {
    printf("\"list\":[");
    for (int i = 0; i < vo->vlist.nb; i++) {
      vo_repr(&vo->vlist.data[i]);
    }
    printf("], ");
  } break;
  } // switch
  printf("}");
}

vo_t *vo_create_list_from_il(vo_initializer_list il) {
  vo_t *res = vo_create(vot_list);
  vo_t *tmp = NULL;
  while (il && !(il->type == vot_string && il->str == NULL)) {
    switch (il->type) {
    case vot_bool:
    case vot_u8: {
      tmp = vo_create(il->type);
      tmp->u8 = il->u8;
    } break;

    case vot_u32: {
      tmp = vo_create(il->type);
      tmp->u32 = il->u32;
    } break;

    case vot_u64: {
      tmp = vo_create(il->type);
      tmp->u64 = il->u64;
    } break;

    case vot_mpint: // do not expect this
    case vot_string:
    case vot_namelist: {
      tmp = vo_create(il->type);
      vstr_iaddpc(&tmp->vstr, il->str);

    } break;

    case vot_list: {
      tmp = vo_create_list_from_il(il->list);
    } break;
      // default:
    } // switch

    if (tmp)
      vlist_append(&res->vlist, tmp);
    free(tmp);
    tmp = NULL;

    il++;
  }

  return res;
}