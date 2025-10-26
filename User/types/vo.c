#include "vo.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "allocator.h"
#include "log.h"

#define ALLOC_ERROR 1
#define UNREACHABLE 1

// variable length string
// behavior notes: all manipulation ensures null terminated or NULL; all
// parameters are assumed valid(not NULL); for string, always reserved>=len+1 or
// data=NULL; data=NULL equals to reserved=0, induces len=0, reserved equals
// size of data; always reserved < STRMAX(1<<20);

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
  vstr_reserve(str, str->len + len);
  memcpy(&str->data[str->len], pc, len);
  str->len += len;
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
  memcpy(&list->data[list->nb], vo, sizeof(vo_t));
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
    vstr_copy(&src->vstr, &dst->vstr);
    break;
  case vot_list:
    vlist_copy(&src->vlist, &dst->vlist);
    break;
  default:
    assert(0 && UNREACHABLE);
  }
}

vstr_t *vo_tobuff(const vo_t *vo) {
  if (!is_packet(vo)) {
    printf("vo_tobuff: not a valid packet\r\n");
    return NULL;
  }

  vstr_t *res = vstr_create(256);
  for (int i = 0; i < vo->vlist.nb; i++) {
    const vo_t *item = &vo->vlist.data[i];
    int nl_size; // to collect size of namelist

    switch (item->type) {
    case vot_bool:
    case vot_u8:
      vbuff_iaddc(res, item->u8);
      break;
    case vot_u32:
      vbuff_iaddu32(res, item->u32);
      break;
    case vot_u64:
      vbuff_iaddu64(res, item->u64);
      break;
    case vot_string:
    case vot_mpint:
      vbuff_iaddu32(res, item->vstr.len);
      vbuff_iadd(res, item->vstr.data, item->vstr.len);
      break;
    case vot_list:
      nl_size = item->vlist.nb ? item->vlist.nb - 1 : 0;
      for (int j = 0; j < item->vlist.nb; j++) {
        nl_size += item->vlist.data[j].vstr.len;
      }
      vbuff_iaddu32(res, nl_size);
      for (int j = 0; j < item->vlist.nb; j++) {
        if (j != 0)
          vbuff_iaddc(res, ',');
        const vstr_t *name = &item->vlist.data[j].vstr;
        vbuff_iadd(res, name->data, name->len);
      }
      break;
    default:
      assert(0 && UNREACHABLE);
    }
  }
  return res;
}