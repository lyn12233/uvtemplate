#include "crypto_api.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

void arc4random_buf(void *buf, size_t nbytes) {
  //   uint8_t *pbuff = buf;
  size_t i;
  for (i = 0; i * 4 < nbytes; i++) {
    ((uint32_t *)buf)[i] = arc4random();
  }
  for (i = i * 4; i < nbytes; i++) {
    ((uint8_t *)buf)[i] = arc4random();
  }
}

uint32_t arc4random(void) {
  static uint32_t seed = 1;
  seed = seed * 0xfefefef3;
  return seed;
}