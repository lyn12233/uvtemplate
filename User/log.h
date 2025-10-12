///@brief overrides printf from microlib
// #pragma once // to override

// non-overriding part
#ifndef _USER_LOG_H_
#define _USER_LOG_H_

#include "stm32f1xx_hal.h"
typedef struct {
  char str[20];
} buff20_t;

buff20_t itoa(int val, unsigned char radix);
buff20_t utoa(unsigned val, unsigned char radix);
buff20_t lltoa(long long val, unsigned char radix);
buff20_t ulltoa(unsigned long long val, unsigned char radix);
buff20_t lftoa(double val, char width, char precision, char fill);

#endif

// overriding part

#include "initors.h"
#include "stm32f1xx_hal_usart.h"

#include <stdio.h>
#include <string.h>

int printf_v2(const char *s, ...);
int puts_v2(const char *s);
int putchar_v2(int val);

#define printf printf_v2
#define puts puts_v2
#define CHECK_FAIL(x)                                                          \
  {                                                                            \
    if (!(x)) {                                                                \
      while (1) {                                                              \
        printf_v2("assertion failed at %s:%d : %s", __FILE__, __LINE__, #x);   \
        puts_v2("");                                                           \
        HAL_Delay(1000);                                                       \
      }                                                                        \
    }                                                                          \
  }
#define assert(x) CHECK_FAIL(x)