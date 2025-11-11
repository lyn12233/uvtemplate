#include "port_socket.h"

#include <stdint.h>

typedef union {
  uint64_t val;
  uint8_t bytes[8];
} swap64_t;

typedef union {
  uint32_t val;
  uint8_t bytes[4];
} swap32_t;

typedef union {
  uint16_t val;
  uint8_t bytes[2];
} swap16_t;

uint64_t htonll(uint64_t hostlong) {
  swap64_t src, dst;
  src.val = hostlong;
  for (uint8_t i = 0; i < 8; i++) {
    dst.bytes[i] = src.bytes[7 - i];
  }
  return dst.val;
}
uint32_t htonl(uint32_t hostlong) {
  swap32_t src, dst;
  src.val = hostlong;
  for (uint8_t i = 0; i < 4; i++) {
    dst.bytes[i] = src.bytes[3 - i];
  }
  return dst.val;
}
uint16_t htons(uint16_t hostshort) {
  swap16_t src, dst;
  src.val = hostshort;
  for (uint8_t i = 0; i < 2; i++) {
    dst.bytes[i] = src.bytes[1 - i];
  }
  return dst.val;
}
uint64_t ntohll(uint64_t hostlong) { return htonll(hostlong); }
uint32_t ntohl(uint32_t netlong) { return htonl(netlong); }
uint16_t ntohs(uint16_t netshort) { return htons(netshort); }